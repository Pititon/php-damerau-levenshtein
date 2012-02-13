#ifndef PHP_DAMERAU_H
#define PHP_DAMERAU_H 1
#define PHP_DAMERAU_VERSION "1.0"
#define PHP_DAMERAU_EXTNAME "damerau"

PHP_FUNCTION(damerau_levenshtein);

extern zend_module_entry damerau_module_entry;
#define phpext_damerau_ptr &damerau_module_entry
#endif
