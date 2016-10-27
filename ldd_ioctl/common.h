

typedef unsigned char 		u8;
typedef unsigned short 		u16;
typedef unsigned int 		u32;
typedef unsigned long long 	u64;

// Choose 44 as signal number (real-time signals are in the range of 33 to 64)
#define SIG_READ 44


/* Use 'K' as magic number */
#define GCHAR_IOC_MAGIC  'G'

struct ioctl_data_t {
	pid_t pid;
	// dma descriptors
};

#define GCHAR_IOC_RESET   _IO(GCHAR_IOC_MAGIC,  1)
#define GCHAR_IOC_WRITE   _IOW(GCHAR_IOC_MAGIC, 2, struct ioctl_data_t)
#define GCHAR_IOC_READ    _IOR(GCHAR_IOC_MAGIC, 3, struct ioctl_data_t)

#define GCHAR_IOC_MAXNR 3