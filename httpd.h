#include <microhttpd.h>
#define HTTPDPORT 4445
#ifdef __cplusplus
extern "C" {
#endif
int
answer_to_connection (void *cls, struct MHD_Connection *connection,
                      const char *url, const char *method,
                      const char *version, const char *upload_data,
                      size_t *upload_data_size, void **con_cls);
					  
void setTemperature(double temp);
#ifdef __cplusplus
} 
#endif