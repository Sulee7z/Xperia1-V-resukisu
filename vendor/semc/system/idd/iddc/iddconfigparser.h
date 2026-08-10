/*
 * Copyright 2011 Sony Corporation
 */

/**
 * @brief IDD Configuration Parser Handling
 *
 * @author Fengjuan Yao (fengjuan3.yao@sonyericsson.com)
 */
#ifndef IDDCCONFIGPARSER_H_INCLUSION_GUARD
#define IDDCCONFIGPARSER_H_INCLUSION_GUARD

/*
 * Parse all idd configuration protobuf file
 * parts and print to stdout.
 */
void parse_idd_config(IDD_t* idd);

unsigned char* read_entire_file(int fd, size_t* size_p);

void parse_config_request(unsigned char* cr, int cr_size);

#endif
