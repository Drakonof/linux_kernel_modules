#if defined(IOCTL_DEMO_H) == 0
#define IOCTL_DEMO_H

#ifdef __cplusplus
extern "C" {
#endif

struct demo_struct {
    int repeat;
    char name[64];
};

/* 
The type argument is a unique identifier that helps differentiate and 
categorize IOCTL commands. It is typically a single character or a number. 
In the given code snippet, the character constant 'a' is used as the type 
argument for all three IOCTL commands (WR_VALUE, RD_VALUE, and GREETER). 
This means that these commands belong to the same category or device.
*/
#define WR_VALUE _IOW('a', 'a', int32_t *)
#define RD_VALUE _IOR('a', 'b', int32_t *)
#define GREETER _IOW('a', 'c', struct demo_struct *)

#ifdef __cplusplus
}
#endif

#endif