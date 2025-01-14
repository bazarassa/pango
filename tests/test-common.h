#ifndef __TEST_COMMON_H__
#define __TEST_COMMON_H__

char * diff_with_file (const char  *file,
                       char        *text,
                       gssize       len,
                       GError     **error);

void print_attribute (VogueAttribute *attr,
                      GString        *string);

void print_attributes (GSList        *attrs,
                       GString       *string);

void print_attr_list (VogueAttrList  *attrs,
                      GString        *string);

const char *get_script_name (GUnicodeScript s);


#endif
