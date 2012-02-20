--TEST--
Bug #1 (Unicode support)
--SKIPIF--
<?php extension_loaded('damerau') or die('skip damerau not available');?>
--FILE--
<?php
var_dump(damerau_levenshtein("123", "12"));
var_dump(damerau_levenshtein("qwe", "qwa"));
var_dump(damerau_levenshtein("фыв", "фыа"));
?>
--EXPECT--
int(1)
int(1)
int(1)