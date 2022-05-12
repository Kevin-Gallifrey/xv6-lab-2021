# Large files
参照singly-indirect，建立doubly-indirect。需要读取两次block找到对应的on-disk block number。  
最大文件大小：11(direct) + 1 * 256(singly-indirect) + 256 * 256(doubly-indirect) blocks

# Symbolic links
`sys_symlink`负责创建一个inode，类型为T_SYMLINK(`create`)，并将target写入其中(`writei`)。  
`sys_open`需要对T_SYMLINK类型文件进行另外处理，通过读取其中内容(`readi`)，找到真实文件的位置，并打开。  
如果有O_NOFOLLOW标记，则直接打开symlink文件。