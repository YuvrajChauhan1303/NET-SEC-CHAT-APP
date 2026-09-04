void send_command(int s, char *buf, unsigned char *aes_key);
int receive_command(int s, unsigned char *aes_key, char *buf, int buf_size);
int read_full(int fd, void *buf, size_t len);
int write_full(int fd, const void *buf, size_t len);