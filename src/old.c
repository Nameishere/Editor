// void filePath(OutputData *E) {
//  
//     DIR *pDir;
//
//     struct dirent *pDirent;
//
//
//     pDir = opendir(".");
//
//     if (pDir == NULL) {
//         die("Cannot Open Directory\\n");
//     }
//
//     char line[258];  
//     while ((pDirent = readdir(pDir)) != NULL ) {
//
//         sprintf(line,  "[%s]", pDirent->d_name);
//         // printf(line);
//
//         editorInsertRow(E->numrows, line, strlen(line), E);
//     }
//     closedir(pDir);
//
//     E->dirty = 0;
//}
