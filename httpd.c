#include <string.h>
#include <stdio.h>
#include "httpd.h"

double temperature = -13.13;
char buffer[1024];
const char *error = "404 There is not such page";

int
answer_to_connection (void *cls, struct MHD_Connection *connection,
                      const char *url, const char *method,
                      const char *version, const char *upload_data,
                      size_t *upload_data_size, void **con_cls)
{
  
  struct MHD_Response *response;
  int ret;
  
  if(0==strcmp(url,"/temp")) {
	  sprintf(buffer, "DS18B20 Temperature=%.2fC", temperature);
      response = MHD_create_response_from_buffer (strlen (buffer), (void *) buffer, MHD_RESPMEM_PERSISTENT);
      MHD_add_response_header (response, MHD_HTTP_HEADER_CONTENT_TYPE, "text/plain");
      ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
      MHD_destroy_response (response);
  }
  else {
	  response = MHD_create_response_from_buffer (strlen (error), (void *) error, MHD_RESPMEM_PERSISTENT);
      MHD_add_response_header (response, MHD_HTTP_HEADER_CONTENT_TYPE, "text/plain");
      ret = MHD_queue_response (connection, MHD_HTTP_NOT_FOUND, response);
      MHD_destroy_response (response);
	  
  }

  return ret;
}

void setTemperature(double temp) {
	temperature=temp;
}