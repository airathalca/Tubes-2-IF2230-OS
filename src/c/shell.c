#include "header/shell.h"

void shell() {
  char input_buf[64];
  char path_str[128];
  char command[32];
  char arg1[32];
  char arg2[32];
  enum fs_retcode ret_code = 0;
  byte current_dir = FS_NODE_P_IDX_ROOT;

  while (true) {
    clear(input_buf, 64);
    clear(arg1, 32);
    clear(arg2, 32);
    clear(command, 32);
    printString("user@uSUSbuntuOS:");
    printCWD(path_str, current_dir);
    printString("$ ");
    readString(input_buf);
    argSplitter(&input_buf, &command, &arg1, &arg2);
    command_type(&command, &current_dir, &arg1, &arg2, &ret_code);
  }
}

void command_type(char *command, byte *current_dir, char* arg1, char* arg2, enum fs_retcode *ret_code){
  if (strcmp(command, "cd")) {
    cd(current_dir, arg1, &ret_code);
  } 

  else if(strcmp(command, "clear")){
    clearScreen();
    return;
  }

  else if (strcmp(command, "ls")) {
    ls(*current_dir, arg1, &ret_code);
  } 

  else if (strcmp(command, "mv")) {
    mv(*current_dir, arg1, arg2, &ret_code);
  }

  else if (strcmp(command, "mkdir")) {
    mkdir(*current_dir, arg1, &ret_code);
  } 

  else if (strcmp(command, "cat")) {
    cat(*current_dir, arg1, &ret_code);
  } 
  else if (strcmp(command, "cp")) {
    cp(*current_dir, arg1, arg2, &ret_code);
  }
  else {
    printString("Unknown command\r\n");
    return;
  }

  error_code(ret_code, command, arg1, arg2);
}

void argSplitter(char *input_buf, char *command, char* arg1, char *arg2){
  int size;
  int i = 0;
  int now = 0;
  int k;
  int count = 0;
  
  while(input_buf[i] != '\0') {
    if(count == 0){
      k = 0;
      while(input_buf[i] != ' ' && input_buf[i] != '\0'){
        command[k] = input_buf[i];
        k++;
        i++;
      }
      count++;
    }else if(count == 1){
      k = 0;
      while(input_buf[i] != ' ' && input_buf[i] != '\0'){
        arg1[k] = input_buf[i];
        k++;
        i++;
      }
      count++;
    }else if(count == 2){
      k = 0;
      while(input_buf[i] != ' ' && input_buf[i] != '\0'){
        arg2[k] = input_buf[i];
        k++;
        i++;
      }
      count++;
    }
    i++;
  }
}

void cd(byte *parentIndex, char *dir, enum fs_retcode *ret_code) {
  struct node_filesystem node_fs_buffer;
  int i;
  int cur_idx = *parentIndex;

  readSector(&(node_fs_buffer.nodes[0]), FS_NODE_SECTOR_NUMBER);
	readSector(&(node_fs_buffer.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);
  
  // kalo gaada dir nya
  if(dir[0] == '\0'){
    *ret_code = FS_R_NODE_NOT_FOUND;
    return;
  } else if(dir[0] == '/'){
    *parentIndex = read_relative_path(FS_NODE_P_IDX_ROOT, dir + 1, ret_code);
    return;

  } else {
    *parentIndex = read_relative_path(*parentIndex, dir, ret_code);
  }
}

void ls(byte parentIdx, char* arg1, enum fs_retcode *ret_code) {
  struct node_filesystem node_fs_buffer;
  int i = 0;
  byte parentFound = FS_NODE_P_IDX_ROOT;

  readSector(&(node_fs_buffer.nodes[0]), FS_NODE_SECTOR_NUMBER);
  readSector(&(node_fs_buffer.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

  if(arg1[0] == '\0'){
    while (i < 64) {
      if (strlen(node_fs_buffer.nodes[i].name) > 0 && node_fs_buffer.nodes[i].parent_node_index == parentIdx) {
        printString(node_fs_buffer.nodes[i].name);
        printString("\r\n");
      }
      i++;
    }
    *ret_code = FS_SUCCESS;
    return;
  }
  //kalo arg1 nya ga nol berarti ini nyari dulu node yg namanya sama
  for(i = 0; i < 64; i++) {
    if (strlen(node_fs_buffer.nodes[i].name) > 0 && strcmp(node_fs_buffer.nodes[i].name, arg1) && node_fs_buffer.nodes[i].parent_node_index == parentIdx){
      parentFound = i;
      break;
    }
  }

  if(i == 64){
    *ret_code = FS_W_INVALID_FOLDER;
    return;
  }

  //cari lagi sesuai parent idx yang baru
  for (i = 0; i < 64; i++) {
    if (strlen(node_fs_buffer.nodes[i].name) > 0 && node_fs_buffer.nodes[i].parent_node_index == parentFound){
      printString(node_fs_buffer.nodes[i].name);
      printString("\r\n");
    }
  }

  *ret_code = FS_SUCCESS;
  return;
}

void mv(byte parentIndex, char *source, char *target, enum fs_retcode *ret_code) {
  char buffer[8192];
  struct node_filesystem node_fs_buffer;
  struct file_metadata *fileInfo;
  byte ROOT = FS_NODE_P_IDX_ROOT;
  int i;
  int j;
  if (!checkArgs(source,ret_code)) {
    return;
  }
  clear(buffer, 8192);
  readSector(&(node_fs_buffer.nodes[0]), FS_NODE_SECTOR_NUMBER);
  readSector(&(node_fs_buffer.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);
  for(i = 0; i < 64; i++){
    if(node_fs_buffer.nodes[i].parent_node_index == parentIndex && strcmp(node_fs_buffer.nodes[i].name, source)){
      break;
    }
  }
  if (i == 64) {
    *ret_code = FS_R_NODE_NOT_FOUND;
    return;
  }
  memcpy(fileInfo->buffer, buffer, 8192);
  strcpy(fileInfo->node_name, source);
  fileInfo->parent_index = parentIndex;
  read(fileInfo, ret_code);
  if (*ret_code != FS_SUCCESS && *ret_code != FS_R_TYPE_IS_FOLDER) {
    return;
  }
  if(target[0] == '/'){
    fileInfo->parent_index = FS_NODE_P_IDX_ROOT;
    if(strlen(target+1) > 13){
      *ret_code = FS_W_NOT_ENOUGH_STORAGE;
      return;
    }
    if(target[1] != '\0'){
      strcpy(fileInfo->node_name, target+1);
    }
    node_fs_buffer.nodes[i].parent_node_index = FS_NODE_P_IDX_ROOT;
  }
  else if(target[0] == '.' && target[1] == '.' && target[2] == '/'){
    if(strlen(target+3) > 13){
      *ret_code = FS_W_NOT_ENOUGH_STORAGE;
      return;
    }
    if(target[3] != '\0'){
      strcpy(fileInfo->node_name, target+3);
    }
    if(parentIndex != FS_NODE_P_IDX_ROOT){
      ROOT = node_fs_buffer.nodes[parentIndex].parent_node_index;
    }
    fileInfo->parent_index = ROOT;
    node_fs_buffer.nodes[i].parent_node_index = ROOT;
  }
  else {
  j = 0;
  while (j < 64) {
    if (strcmp(node_fs_buffer.nodes[j].name, target) && parentIndex == node_fs_buffer.nodes[j].parent_node_index){
      if (node_fs_buffer.nodes[j].sector_entry_index == FS_NODE_S_IDX_FOLDER) {
        break;
      }
    } 
    else {
      j++;
    }
  }
  if (j == 64) {
    *ret_code = FS_W_INVALID_FOLDER;
    return;
  }
  if(strlen(target) > 13){
    *ret_code = FS_W_NOT_ENOUGH_STORAGE;
    return;
  }
  fileInfo->parent_index = j;
  node_fs_buffer.nodes[i].parent_node_index = j;
}
  strcpy(node_fs_buffer.nodes[i].name, fileInfo->node_name);
  writeSector(&(node_fs_buffer.nodes[0]), FS_NODE_SECTOR_NUMBER);
  writeSector(&(node_fs_buffer.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

  *ret_code = FS_SUCCESS;
  return;
}

bool checkArgs(char *filename, int *ret_code) {
  if(filename[0] == '\0'){
      *ret_code = FS_R_NODE_NOT_FOUND;
      return false;
    }
  if(strlen(filename) > 13){
    *ret_code = FS_W_NOT_ENOUGH_STORAGE;
    return false;
  }
  return true;
}

void cat(byte parentIndex, char *filename, enum fs_retcode *ret_code) {
  //diketahui parentIndexnya trs tinggal searching node mana yang p nya sama 
  // berarti itu ada di folder tsb cek namanya sama apa ga
  struct file_metadata *fileInfo;
  int i = 0;
  if (!checkArgs(filename,ret_code)) {
    return;
  }
  fileInfo->parent_index = parentIndex;
  strcpy(fileInfo->node_name, filename);

  read(fileInfo, ret_code);
  if(*ret_code == 0){
    printString(fileInfo->buffer);
    printString("\r\n");
  }
}

void mkdir(byte cur_dir_idx, char *arg1, enum fs_retcode *ret_code){
  //cek dulu apakah ada folder yang namanya sama
  struct file_metadata *fileinfo;
  if (!checkArgs(arg1,ret_code)) {
    return;
  }
  fileinfo->parent_index = cur_dir_idx;
  strcpy(fileinfo->node_name, arg1);
  //udah ada isinya si fileinfonya;
  write(fileinfo, ret_code);
}

void cp(byte parentIdx, char *resourcePath, char *destinationPath, enum fs_retcode *ret_code){
  char buffer[8192];
  struct node_filesystem node_fs_buffer;
  struct file_metadata *fileInfo;
  int i;
  clear(buffer, 8192);
  readSector(&(node_fs_buffer.nodes[0]), FS_NODE_SECTOR_NUMBER);
  readSector(&(node_fs_buffer.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);
  memcpy(fileInfo->buffer, buffer, 8192);
  strcpy(fileInfo->node_name, resourcePath);
  fileInfo->parent_index = parentIdx;
  read(fileInfo, ret_code);
  if (*ret_code != 0) {
    return;
  }

  if(destinationPath[0] == '\0'){
    *ret_code = FS_R_NODE_NOT_FOUND;
    return;

  } else if(destinationPath[0] == '/'){
    i = read_relative_path(FS_NODE_P_IDX_ROOT, destinationPath + 1, ret_code);

  } else {
    i = read_relative_path(parentIdx, destinationPath, ret_code);
  }

  if (i == parentIdx) {
    strcpy(fileInfo->node_name, destinationPath);
    write(fileInfo, ret_code);
    return;
  }

  fileInfo->parent_index = i;
  write(fileInfo, ret_code);
}

void printCWD(char* path_str, byte current_dir) {
  int i = 0;
  int nodeCount = 0;
  int curlen = 0;
  byte nodeIndex[64];
  byte parent = 0;
  byte filename[16];
  bool isTruncated;
  struct node_filesystem node_fs_buffer;
  struct sector_filesystem sector_fs_buffer;

  readSector(&node_fs_buffer, FS_NODE_SECTOR_NUMBER);
  readSector(&node_fs_buffer.nodes[32], FS_NODE_SECTOR_NUMBER + 1);
  readSector(&sector_fs_buffer, FS_SECTOR_SECTOR_NUMBER);
  //kosongin dulu
  clear(path_str, 128);
  clear(nodeIndex, 64);
  if(current_dir == FS_NODE_P_IDX_ROOT){
    path_str[curlen++] = '/';
    printString(path_str);
    return;
  }
  //masukan current dir ke array of node index
  //misal skrg di /a/b
  // /a/b/c
  nodeIndex[nodeCount] = current_dir;
  nodeCount++;
  //loop sampe parentnya oxFF
  parent = node_fs_buffer.nodes[current_dir].parent_node_index;
  while(parent != FS_NODE_P_IDX_ROOT){
    nodeIndex[nodeCount] = parent;
    parent = node_fs_buffer.nodes[parent].parent_node_index;
    nodeCount++;
  }
  
  isTruncated = false;
  while(nodeCount > 0 && curlen < 128){
    nodeCount--;
    path_str[curlen++] = '/';
    strcpy(filename, node_fs_buffer.nodes[nodeIndex[nodeCount]].name);

    if(curlen + strlen(filename) > 128){
      isTruncated = true;
      break;
    }

    for(i = 0; i < strlen(filename); i++){
      path_str[curlen++] = filename[i];
    }
  }

  if (isTruncated) {
    printString("../");
    printString(node_fs_buffer.nodes[nodeIndex[1]].name);
    printString("/");
    printString(node_fs_buffer.nodes[nodeIndex[0]].name);
    return;
  }

  printString(path_str);
}

byte read_relative_path(byte parentIdx, char *path_str, enum fs_retcode *ret_code) {
  char temp_str[128];
  struct node_filesystem node_fs_buffer;
  int i = 0;
  int j = 0;
  int k;
  bool found = false;
  byte prevParentIndex = parentIdx;

  readSector(&(node_fs_buffer.nodes[0]), FS_NODE_SECTOR_NUMBER);
	readSector(&(node_fs_buffer.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

  clear(temp_str, 128);

  while (path_str[i] != '\0') {
    if(path_str[i] == '/') {
      i++; 
      continue;
    }

    j = 0;
    while (path_str[i] != '\0' && path_str[i] != '/'){
      temp_str[j] = path_str[i];
      i++;
      j++;
    }

    if (strcmp(temp_str, ".")) {
      clear(temp_str, 128);

    } else if (strcmp(temp_str, "..")) {
      if (parentIdx == FS_NODE_P_IDX_ROOT){
        *ret_code = FS_W_INVALID_FOLDER;
        return prevParentIndex;
      }

      parentIdx = node_fs_buffer.nodes[parentIdx].parent_node_index;
      clear(temp_str, 128);

    } else {
      found = false;
      for (k = 0; k < 64 && !found; k++){
        if(strcmp(temp_str, node_fs_buffer.nodes[k].name) && node_fs_buffer.nodes[k].parent_node_index == parentIdx){ // SALAH DISINI
          parentIdx = k;
          found = true;
        }
      }

      if(node_fs_buffer.nodes[parentIdx].sector_entry_index != FS_NODE_S_IDX_FOLDER) {
        *ret_code = FS_R_TYPE_IS_FOLDER;
        return prevParentIndex;

      } else if (!found){
        *ret_code = FS_W_INVALID_FOLDER;
        return prevParentIndex;

      } else {
        clear(temp_str, 128);
      }
    }
  }

  *ret_code = FS_SUCCESS;
  return parentIdx;
}

void error_code(int err_code, char*command, char*arg1, char*arg2){
  int arg1_len = 0;
  int arg2_len = 0;
  if(err_code != 0){
    printString(command);
    printString(": ");
  }
  arg1_len = strlen(arg1);
  arg2_len = strlen(arg2);
  switch (err_code)
  {
  case -1:
    printString("Unknown Error\r\n");
    break;
  case 1:
    printString(arg1);
    if(arg1_len) printString(" ");
    printString("File not found\r\n");
    break;
  case 2:
    printString(arg1);
    if(arg1_len) printString(" ");
    printString("Is a directory\r\n");
    break;
  case 3:
    printString(arg2);
    if(arg2_len) printString(" ");
    printString(" File already exists\r\n");
    break;
  case 4:
    printString("Storage is full\r\n");
    break;
  case 5:
    printString("Maximum file capacity achieved\r\n");
    break;
  case 6:
    printString("Maximum sector capacity achieved\r\n");
    break;
  case 7:
    printString(arg1);
    if(arg1_len) printString(" ");
    printString(arg2);
    if(arg2_len) printString(" ");
    printString("No such directory exists\r\n");
    break;
  case 8:
    printString(arg1);
    if(arg1_len) printString(" ");
    printString("Folder already exists\r\n");
  default:
    break;
  }
}