#ifndef LIB
#define LIB
#define max_len 250
#define time 5

typedef struct {
    int len;
    char payload[1400];
} msg;

typedef struct {
	unsigned char SOH, LEN, SEQ, TYPE;
	char DATA[max_len];
}mini_k1;

typedef struct{
	unsigned short CHECK;
	unsigned char MARK;
}mini_k2;

typedef struct{
	unsigned char MAXL, TIME, NPAD, PADC,
	EOL, QCTL, QBIN, CHKT, REPT, CAPA, R;
} s_data;

void init(char* remote, int remote_port);
void set_local_port(int port);
void set_remote(char* ip, int port);
int send_message(const msg* m);
int recv_message(msg* r);
msg* receive_message_timeout(int timeout); //timeout in milliseconds
unsigned short crc16_ccitt(const void *buf, int len);

#endif

