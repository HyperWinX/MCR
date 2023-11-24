# MCR
Really simple C program for creating powerful macroses.
---
### Syntax
```mcr install``` - install program into your bin folder  
```mcr list``` - display a list of available macroses  
```mcr addmacro smth "more more smth"``` - add macros  
```mcr remove smth``` - remove macros "smth"
---
### Why this was created:
Why do you need that? I dont know. This thingy replaces your first word (always its command name) with defined macros. Yea... Like, if you create macros gcc="gcc -O2 -march=native", every time when you call ```mcr gcc main.c```, it will be replaced to ```gcc -O2 -march=native main.c```... Yup, that's all this program does.
