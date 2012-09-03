#include <microhttpd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static ssize_t
read_callback(void *data, uint64_t pos, char *buf, size_t max)
{
  int fd = data;
  int ret;
  ret = read(fd, buf, max);

  if (ret == 0)
    return MHD_CONTENT_READER_END_OF_STREAM;
  if (ret == -1)
    return MHD_CONTENT_READER_END_WITH_ERROR;

  return ret;
}

static void
free_callback(void *data)
{
  int fd = data;
  close(fd);
}

static int stdin_echo(void * cls,
		    struct MHD_Connection * connection,
		    const char * url,
		    const char * method,
                    const char * version,
		    const char * upload_data,
		    size_t * upload_data_size,
                    void ** ptr) {
  static int dummy;
  const char * page = cls;
  struct MHD_Response * response;
  int ret;

  if (0 != strcmp(method, "GET"))
    return MHD_NO; /* unexpected method */

  if (&dummy != *ptr) {
      /* The first time only the headers are valid,
         do not respond in the first round... */
      *ptr = &dummy;
      return MHD_YES;
  }

  if (0 != *upload_data_size)
    return MHD_NO; /* upload data in a GET!? */

  *ptr = NULL; /* clear context pointer */
  response = MHD_create_response_from_callback(MHD_SIZE_UNKNOWN,
               32 * 1024,
               &read_callback,
               STDIN_FILENO,
               &free_callback);
  ret = MHD_queue_response(connection,
			   MHD_HTTP_OK,
			   response);

  MHD_destroy_response(response);
  return ret;
}

int main(int argc, char ** argv) {
  struct MHD_Daemon * d;
  fd_set fds[3];
  int max;
  struct timeval timeout;
  

  if (argc != 2) {
    printf("%s PORT\n",
	   argv[0]);
    return 1;
  }

  d = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY,
		       atoi(argv[1]),
		       NULL,
		       NULL,
		       &stdin_echo,
		       NULL,
		       MHD_OPTION_END);

  FD_ZERO(&fds[0]);
  FD_ZERO(&fds[1]);
  FD_ZERO(&fds[2]);
  FD_SET(STDIN_FILENO, &fds[0]);

  timeout.tv_sec  = 3 * 60;
  timeout.tv_usec = 0;

  do {
    MHD_get_fdset(d, &fds[0], &fds[1], &fds[2], &max);
    MHD_run(d);
  } while (select(max, &fds[0], &fds[1], &fds[2], &timeout)); 

  return 0;
}
