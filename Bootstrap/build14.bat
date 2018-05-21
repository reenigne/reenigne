@echo off

if not exist 13.com call build13.bat

echo Building step 14

13 14util.13 14lex.13 14parse.13 14expr.13 14malloc.13 14.com
