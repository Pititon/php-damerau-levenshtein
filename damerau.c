#ifdef HAVE_CONFIG_H

#include "config.h"

#endif





#include "php.h"
#include "php/Zend/zend_API.h"
#include "php_ini.h"
#include "php_damerau.h"
#include "php/ext/mbstring/libmbfl/mbfl/mbfl_string.h"
#include "php/ext/mbstring/libmbfl/mbfl/mbfl_language.h"
#include "php/ext/mbstring/libmbfl/mbfl/mbfilter.h"
#include "php/main/php.h"


ZEND_BEGIN_ARG_INFO(arginfo_damerau_levenshtein, 0)
    ZEND_ARG_INFO(0, str1)
    ZEND_ARG_INFO(0, str2)
    ZEND_ARG_INFO(0, cost_ins)
    ZEND_ARG_INFO(0, cost_sub)
    ZEND_ARG_INFO(0, cost_del)
    ZEND_ARG_INFO(0, cost_tran)
ZEND_END_ARG_INFO()

static function_entry damerau_levenshtein_functions[] = {

  PHP_FE(damerau_levenshtein, arginfo_damerau_levenshtein)

  {NULL, NULL, NULL}

};

zend_module_entry damerau_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
  STANDARD_MODULE_HEADER,
#endif
  PHP_DAMERAU_EXTNAME,
  damerau_levenshtein_functions,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
#if ZEND_MODULE_API_NO >= 20010901
  PHP_DAMERAU_VERSION,
#endif
  STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_DAMERAU

ZEND_GET_MODULE(damerau)

#endif

#define LEVENSHTEIN_MAX_LENGTH 255

/* {{{ reference_damerau_levenshtein
 * reference implementation, only optimized for memory usage and speed */
static int reference_damerau_levenshtein(const char *first, int firstLength, const char *second, int secondLength, int cost_ins, int cost_sub, int cost_del, int cost_tran )
{
    int *currentRow, *previousRow, *transpositionRow, *tempRow;
    int i, j, t, from, to, cost, value;
    char lastSecondCh = 0, secondCh, firstCh, lastFirstCh;
    
    if (firstLength == 0) {
        return secondLength * cost_ins;
    }
    if (secondLength == 0) {
        return firstLength * cost_del;
    }
    
    if ((firstLength > LEVENSHTEIN_MAX_LENGTH) || (secondLength > LEVENSHTEIN_MAX_LENGTH)) {
        return -1;
    }
    
    if(firstLength > secondLength){
        char *tmp = first;
        first = second;
        second = tmp;
        int tmpl = secondLength;
        secondLength = firstLength;
        firstLength = tmpl;
    }

    currentRow = safe_emalloc((firstLength + 1), sizeof(int), 0);
    previousRow = safe_emalloc((firstLength + 1), sizeof(int), 0);
    transpositionRow = safe_emalloc((firstLength + 1), sizeof(int), 0);
    
    for (i=0; i<=firstLength; i++){
        previousRow[i] = i * cost_ins;
        currentRow[i] = 0;
        transpositionRow[i] = 0;
    }
    
    for(i = 1; i <= secondLength; i++){
        secondCh = second[i-1];
        currentRow[0] = i;
        
        from = i-secondLength - 1;
        if(from < 1) from = 1;
        to = i+secondLength + 1;
        if(to > firstLength) to = firstLength;
        
        lastFirstCh = 0;
        for(j=from; j <= to; j++){
            firstCh = first[j-1];
            
            cost = (firstCh == secondCh)? 0 : 1;
            value = currentRow[j-1] + cost_ins; // insertion
            t = previousRow[j] + cost_del; // deletion
            if(value > t) value = t;
            t = previousRow[j-1] + (cost?cost_sub:0); // substitution
            if(value > t) value = t;
            
            if(firstCh == lastSecondCh && secondCh == lastFirstCh){
                t = transpositionRow[j-2] + (cost?cost_tran:0); // transposition
                if(value > t) value = t; 
            }
            
            currentRow[j] = value;
            lastFirstCh = firstCh;
        }
        lastSecondCh = secondCh;
        
        tempRow = transpositionRow;
        transpositionRow = previousRow;
        previousRow = currentRow;
        currentRow = tempRow;
    }
    
    cost = previousRow[firstLength];
    efree(currentRow);
    efree(previousRow);
    efree(transpositionRow);
    return cost;
}
/* }}} */

/* {{{ reference_damerau_levenshtein
 * reference implementation, only optimized for memory usage and speed */
static int reference_mb_damerau_levenshtein(
        const char *f, 
        int fl, 
        const char *s, 
        int sl, 
        int cost_ins, int cost_sub, int cost_del, int cost_tran
){
    int *currentRow, *previousRow, *transpositionRow, *tempRow;
    int i, j, t, from, to, cost = 0, value;
    mbfl_string first, second;
    
    int firstLength, secondLength;
    mbfl_string_init(&first);
    first.no_encoding = mbfl_no_encoding_utf8;
    first.val = (unsigned char *)f;
    first.len = fl;
    mbfl_string_init(&second);
    second.no_encoding = mbfl_no_encoding_utf8;
    second.val = (unsigned char *)s;
    second.len = sl;
    
	firstLength = mbfl_strlen(&first);
    secondLength = mbfl_strlen(&second);
    
    if (firstLength == 0) {
        return secondLength * cost_ins;
    }
    if (secondLength == 0) {
        return firstLength * cost_del;
    }
    if ((firstLength > LEVENSHTEIN_MAX_LENGTH) || (secondLength > LEVENSHTEIN_MAX_LENGTH)) {
        return -1;
    }
    if(firstLength > secondLength){
        mbfl_string *tmp = &first;
        first = second;
        second = *tmp;
        int tmpl = secondLength;
        secondLength = firstLength;
        firstLength = tmpl;
    }
    
    currentRow = safe_emalloc((firstLength + 1), sizeof(int), 0);
    previousRow = safe_emalloc((firstLength + 1), sizeof(int), 0);
    transpositionRow = safe_emalloc((firstLength + 1), sizeof(int), 0);

    for (i=0; i<=firstLength; i++){
        previousRow[i] = i * cost_ins;
        currentRow[i] = 0;
        transpositionRow[i] = 0;
    }
    
    mbfl_string *lastSecondCh;
    mbfl_string *lastFirstCh;
    mbfl_string firstCh, *retFirstCh;
    mbfl_string secondCh, *retSecondCh;
    
    mbfl_string_init(&firstCh);
    mbfl_string_init(&secondCh);
    firstCh.no_encoding = mbfl_no_encoding_utf8;
    secondCh.no_encoding = mbfl_no_encoding_utf8;
    
    for(i = 1; i <= secondLength; i++){
        retSecondCh = mbfl_substr(&second, &secondCh, i-1, 1);
        currentRow[0] = i;
        
        from = i-secondLength - 1;
        if(from < 1) from = 1;
        to = i+secondLength + 1;
        if(to > firstLength) to = firstLength;
        
        for(j=from; j <= to; j++){
            retFirstCh = mbfl_substr(&first, &firstCh, j-1, 1);
            cost = strcmp(retFirstCh->val,retSecondCh->val) == 0? 0 : 1;
            value = currentRow[j-1] + cost_ins; // insertion
            t = previousRow[j] + cost_del; // deletion
            if(value > t) value = t;
            t = previousRow[j-1] + (cost?cost_sub:0); // substitution
            if(value > t) value = t;
            
            if(j>1 && (strcmp(retFirstCh->val,lastSecondCh->val) == 0) 
                    && (strcmp(retSecondCh->val,lastFirstCh->val) == 0)
            ){
                t = transpositionRow[j-2] + (cost?cost_tran:0); // transposition
                if(value > t) value = t; 
            }
            
            currentRow[j] = value;
            lastFirstCh = retFirstCh;
            if(retFirstCh != (mbfl_string *)NULL)
                mbfl_string_clear(retFirstCh);
        }

        lastSecondCh = retSecondCh;
        if(retSecondCh != (mbfl_string *)NULL)
            mbfl_string_clear(retSecondCh);
        
        tempRow = transpositionRow;
        transpositionRow = previousRow;
        previousRow = currentRow;
        currentRow = tempRow;
    }
    
    if(lastFirstCh != (mbfl_string *)NULL)
        mbfl_string_clear(lastFirstCh);
    if(lastSecondCh != (mbfl_string *)NULL)
        mbfl_string_clear(lastSecondCh);
    
    cost = previousRow[firstLength];

    efree(currentRow);
    efree(previousRow);
    efree(transpositionRow);
    
    return cost;
    
}

/* }}} */

/* {{{ proto int levenshtein(string str1, string str2[, int cost_ins, int cost_sub, int cost_del, int cost_tran])
 Calculate Damerau - Levenshtein distance between two strings */
PHP_FUNCTION(damerau_levenshtein)
{
    int argc = ZEND_NUM_ARGS();
    char *str1, *str2;
    int str1_len, str2_len;
    long cost_ins, cost_sub, cost_del, cost_tran;
    int distance = -1;
    
    switch (argc) {
        case 2: /* just two strings: use maximum performance version */
            if (zend_parse_parameters(2 TSRMLS_CC, "ss", &str1, &str1_len, &str2, &str2_len) == FAILURE) {
                return;
            }
            distance = reference_mb_damerau_levenshtein(str1, str1_len, str2, str2_len, 1, 1, 1, 1);
            break;
            
        case 6: /* more general version: calc cost by ins/rep/del weights */
            if (zend_parse_parameters(6 TSRMLS_CC, "ssllll", &str1, &str1_len, &str2, &str2_len, &cost_ins, &cost_sub, &cost_del, &cost_tran) == FAILURE) {
                return;
            }
            distance = reference_mb_damerau_levenshtein(str1, str1_len, str2, str2_len, cost_ins, cost_sub, cost_del, cost_tran);
            break;
            
        default:
            WRONG_PARAM_COUNT;
    }
    
    if (distance < 0 && /* TODO */ ZEND_NUM_ARGS() != 3) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "Argument string(s) too long");
    }
    
    RETURN_LONG(distance);
}
/* }}} */

