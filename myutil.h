int str_eql(const char* str1, const char* str2);
int str_eql_t(const char* str1, const char* str2, char terminator);
int str_eql_l(const char* str1, const char* str2, int len);
int str_len(const char* str);
int str_len_t(const char* str, char terminator);
char* str_chr(const char* str, char c);
void str_copy(const char* source, char* dest, int len);
int str_to_int(const char* str, int* num);
int find_in_path(const char* dir, char* out_buffer);
