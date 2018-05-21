@echo off

if not exist 15.com call build15.bat

echo Building step 16

15 16util.15 16lex.15 16parse.15 16symbol.15 16expr.15 16string.15 16pe.15 16malloc.15 16.com
