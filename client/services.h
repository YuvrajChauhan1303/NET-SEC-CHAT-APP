void send_command(int s, char *buf, unsigned char *aes_key);
int receive_command(int s, unsigned char *aes_key, char *buf, int buf_size);