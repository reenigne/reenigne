@echo off

if not exist 14.com call build14.bat

echo Building step 15

14 15util.14 15lex.14 15parse.14 15symbol.14 15expr.14 15string.14 15malloc.14 15.com
