// Minimal C library to interact with the OpenAI API and
// get JSON responses.

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <time.h>

// Internal global variables, not exposed in the header
// to prevent accidental modification by the user
char initialized = 0;
char * api_key = NULL;
char * api_url = "api.openai.com";
char * model = "gpt-4o-mini";
int max_tokens = 100;

// Initializes the neurosymbolic library. This function
// must be called *exactly once* before calling any other
// function in the library.
void neurosym_init() {
    if (initialized) {
        fprintf(stderr, "Already initialized\n");
        return;
    }
    initialized = 1;
    api_key = getenv("OPENAI_API_KEY");
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    srand(time(NULL));
}

// Function to generate a fake response when the API key is not set
// to enable local testing without making actual API calls.
char * fake_response() {
    char * format = "{\
  \"id\": \"chatcmpl-Ag5hb4g6PYiVpvBp3tuiJjqX9KjqN\",\
  \"object\": \"chat.completion\",\
  \"created\": 1734595059,\
  \"model\": \"gpt-4o-mini-2024-07-18\",\
  \"choices\": [\
    {\
      \"index\": 0,\
      \"message\": {\
        \"role\": \"assistant\",\
        \"content\": \"%s\",\
        \"refusal\": null\
      },\
      \"logprobs\": null,\
      \"finish_reason\": \"stop\"\
    }\
  ],\
  \"usage\": {\
    \"prompt_tokens\": 10,\
    \"completion_tokens\": 23,\
    \"total_tokens\": 33,\
    \"prompt_tokens_details\": {\
      \"cached_tokens\": 0,\
      \"audio_tokens\": 0\
    },\
    \"completion_tokens_details\": {\
      \"reasoning_tokens\": 0,\
      \"audio_tokens\": 0,\
      \"accepted_prediction_tokens\": 0,\
      \"rejected_prediction_tokens\": 0\
    }\
  },\
  \"system_fingerprint\": \"fp_6fc10e10eb\"\
}\
";
    char * responses[] = {
        "That’s classified information, but I trust you enough to pretend I don’t know either.",
        "I’d answer that, but I don’t want to ruin the surprise.",
        "Yes, no, maybe – pick one and let’s hope for the best!",
        "My lawyer advised me not to answer that without snacks present.",
        "Funny you should ask – I was just about to Google that myself!",
        "Well, according to the voices in my head, it’s a solid maybe.",
        "I could tell you, but then I’d have to awkwardly dance away without explaining why.",
        "Do you want the correct answer or the one that makes us both laugh?",
        "I’m 99% sure I’m 100% unsure about that.",
        "Let me consult my magic eight-ball oh, it says 'try again later.' Classic eight-ball move!",
    };
    int index = rand() % (sizeof(responses) / sizeof(char *));
    char * response;
    int size = asprintf(&response, format, responses[index]);
    if (size < 0) {
        fprintf(stderr, "Failed to allocate AI response\n");
        exit(1);
    }
    return response;
}

// Function to create and connect a socket to the given hostname and port.
int create_socket(const char *hostname, const char *port) {
    int sockfd;
    struct addrinfo hints, *res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(hostname, port, &hints, &res) != 0) {
        perror("getaddrinfo failed");
        return -1;
    }

    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) {
        perror("socket creation failed");
        freeaddrinfo(res);
        return -1;
    }

    if (connect(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
        perror("connect failed");
        close(sockfd);
        freeaddrinfo(res);
        return -1;
    }

    freeaddrinfo(res);
    return sockfd;
}

// Function to send a prompt to the API and return the JSON response.
// It is the caller's responsibility to free the returned string.
// If anything goes wrong, return NULL.
char * response(const char * prompt) {
    // If library is not initialized, return NULL
    if (!initialized) {
        fprintf(stderr, "Not initialized\n");
        return NULL;
    }
    if (api_key == NULL) {
        return fake_response();
    }
    int sockfd = create_socket(api_url, "443");
    if (sockfd < 0) {
        fprintf(stderr, "Failed to create socket\n");
        return NULL;
    }
    SSL_CTX * ctx = SSL_CTX_new(TLS_client_method());
    if (ctx == NULL) {
        fprintf(stderr, "Failed to create SSL context\n");
        close(sockfd);
        return NULL;
    }
    SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);

    SSL * ssl = SSL_new(ctx);

    if (!SSL_set_tlsext_host_name(ssl, api_url)) {
        fprintf(stderr, "Failed to set SNI\n");
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        close(sockfd);
        return NULL;
    }

    if (ssl == NULL) {
        fprintf(stderr, "Failed to create SSL object\n");
        SSL_CTX_free(ctx);
        close(sockfd);
        return NULL;
    }
    if (!SSL_set_fd(ssl, sockfd)) {
        fprintf(stderr, "Failed to set SSL file descriptor\n");
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        close(sockfd);
        return NULL;
    }
    int connected = SSL_connect(ssl);
    if (connected <= 0) {
        fprintf(stderr, "Failed to connect with SSL %d\n", connected);
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        close(sockfd);
        return NULL;
    }

    // Send the prompt to the API
    char * message, * body;
    int result;
    result = asprintf(
        &body,
        "{\"messages\": [{\"role\": \"user\", \"content\": \"%s\"}], \"model\": \"%s\", \"max_tokens\": %d}",
        prompt,
        model,
        max_tokens
    );
    if (result < 0) {
        fprintf(stderr, "Failed to allocate prompt\n");
        exit(1);
    }
    result = asprintf(
        &message,
        "POST /v1/chat/completions HTTP/1.1\n"
        "Host: %s\n"
        "Authorization: Bearer %s\n"
        "Content-Type: application/json\n"
        "User-Agent: DIT-Neurolib/1.0\n"
        "Accept: application/json\n"
        "Content-Length: %lu\n"
        "\n"
        "%s\n\n",
        api_url,
        api_key,
        strlen(body),
        body
    );
    if (result < 0) {
        fprintf(stderr, "Failed to allocate message\n");
        exit(1);
    }
    int sent = SSL_write(ssl, message, strlen(message));

    free(message);
    free(body);

    if (sent < 0) {
        fprintf(stderr, "Failed to send message\n");
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        close(sockfd);
        return NULL;
    }
    // Receive the entire response with repeated calls to recv + realloc as needed
    char * buffer = NULL;
    int buffer_size = 0;
    int received = 0;
    while (1) {
        char temp[1024];
        int bytes = SSL_read(ssl, temp, sizeof(temp));
        if (bytes <= 0) {
            break;
        }
        buffer = realloc(buffer, buffer_size + bytes);
        if (buffer == NULL) {
            fprintf(stderr, "Failed to allocate recv buffer\n");
            exit(1);
        }
        memcpy(buffer + buffer_size, temp, bytes);
        buffer_size += bytes;
        received += bytes;
    }
    // Null terminate the buffer
    buffer = realloc(buffer, buffer_size + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Failed to allocate recv buffer\n");
        exit(1);
    }
    buffer[buffer_size] = '\0';

    // Filter out HTTP response headers and return only the JSON response
    // JSON response starts with the first '{' character and ends with the last '}'
    char * response = NULL;
    char * start = strchr(buffer, '{');
    char * end = strrchr(buffer, '}');
    if (start != NULL && end != NULL) {
        response = strndup(start, end - start + 1);
    }
    if (response == NULL) {
        fprintf(stderr, "Failed to parse response\n");
    }

    // Free up all state
    free(buffer);
    close(sockfd);
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    return response;
}
