## xargs
`<command1> | <command2>`  
`"|"`是管道符，将`command1`的输出结果当作`command2`的输入。  
由于有些命令不支持管道来传递参数，所以就有了xargs。  
`<command1> | xargs <command2>`  
eg: `echo hello | xargs echo bye` 相当于 `echo bye hello`  