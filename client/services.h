#define BUFFER_SIZE 4096
#define ENCRYPTED_BUFFER_SIZE (BUFFER_SIZE + GCM_IV_SIZE + GCM_TAG_SIZE)

int read_all(int s, void *buf, int len);
int write_all(int s, void *buf, int len);

void send_command(int s, char *buf, unsigned char *aes_key);
int receive_command(int s, unsigned char *aes_key, char *buf, int buf_size);