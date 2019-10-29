#ifndef _REQUESTS_
#define _REQUESTS_

char *compute_request(char *host, char *url, char *url_params, char *form_data, 
        int content_type, char *jwt, char **cookies, int cookie_size, char *method);
char *compute_get_request(char *host, char *url, char *url_params, char *jwt, char **cookies, int cookie_size);
char *compute_post_request(char *host, char *url, char *form_data, int content_type, char *jwt, char **cookies, int cookie_size);

#endif