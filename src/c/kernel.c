// Kode kernel
// PENTING : FUNGSI PERTAMA YANG DIDEFINISIKAN ADALAH main(),
// ek spesifikasi untuk informasi lebih lanjut

// TODO : Tambahkan implementasi kode C

#include "header/kernel.h"

int main() {
    fillKernelMap();
    clearScreen();
    makeInterrupt21();
    welcome();
    shell();
}

void welcome(){
}

void handleInterrupt21(int AX, int BX, int CX, int DX) {
    switch (AX) {
        case 0x0:
            printString(BX);
            break;
        case 0x1:
            readString(BX);
            break;
        case 0x2:
            readSector(BX, CX);
            break;
        case 0x3:
            writeSector(BX, CX);
            break;
        case 0x4:
            read(BX, CX);
            break;
        case 0x5:
            write(BX, CX);
            break;
        default:
            printString("Invalid Interrupt");
    }
}

void printString(char *string) {
    int i = 0;
    char c = string[i];
    byte color = 0xD;
    while (c != '\0') {
        if(c == '\n')interrupt(0x10, 0x0E00 + '\r', 0, 0, 0);
        interrupt(0x10, 0x0E00 + c, 0, 0, 0);
        i++;
        c = string[i];
    }
}

void printHexa(char n)
{
	char number[3];
	number[2] = 0;
	number[0] = div(n, 16);
	number[0] = number[0] >= 10 ? (number[0]-10) + 'A' : number[0] + '0';
	number[1] = mod(n, 16);
	number[1] = number[1] >= 10 ? (number[1]-10) + 'A' : number[1] + '0';
	printString(number);
}

void readString(char *string) {
    bool check = true;
    char c = '\0';
    int i = 0;
    int j = 0;
    while (check) {
        //read character <- user input
        c = interrupt(0x16, 0x0, 0, 0, 0);
        if (c == '\r' || c == '\n') {
            string[i++] = '\0';
            check = false;
            printString("\r\n");
        } else if (c == '\b') {
            if (i > 0){
              interrupt(0x10, 0x0E00 + '\b', 0, 0, 0);
              interrupt(0x10, 0x0E00 + ' ', 0, 0, 0);
              interrupt(0x10, 0x0E00 + '\b', 0, 0, 0);
              string[i] = '\0';
              i--;
            }

        } else if (c == '\t') {
          for(j = 0; j < 4; j++){
            string[i++] = ' ';
            interrupt(0x10, 0x0E00 + ' ', 0, 0, 0);
          }
        } 
        else {
            string[i] = c;
            interrupt(0x10, 0x0E00 + c, 0, 0, 0);

            if(i < 127){
              i++;
            }
        }
    }
}

void clearScreen() {
  interrupt(0x10, 0x0200, 0, 0, 0);
  interrupt(0x10, 0x0600, 0x0f00, 0, 0x184f);
}

void readSector(byte *buffer, int sector_number) {
    interrupt(0x13, 0x0201,buffer,div(sector_number, 36) << 8 | mod(sector_number, 18) + 1,mod(div(sector_number, 18), 2) << 8);
}

void writeSector(byte *buffer, int sector_number) { 
    interrupt(0x13, 0x0301, buffer, div(sector_number, 36) << 8 | mod(sector_number, 18) + 1, mod(div(sector_number, 18), 2) << 8);
}


void fillKernelMap(){
  struct map_filesystem map_fs_buffer;
  int i;
  // Load filesystem map
  readSector(&map_fs_buffer, FS_MAP_SECTOR_NUMBER);
  for(i = 0; i < 512; i++) {
    if(i < 16 || i > 256){
      map_fs_buffer.is_filled[i] = true;
    }
  }
  writeSector(&map_fs_buffer, FS_MAP_SECTOR_NUMBER); 
}

void read(struct file_metadata *metadata, enum fs_retcode *return_code) {
  struct node_filesystem   node_fs_buffer;
  struct node_entry        node_buffer;
  struct sector_filesystem sector_fs_buffer;
  struct sector_entry      sector_entry_buffer;
  int i = 0;
  int j = 0;
  int foundEntryNode = FS_NODE_P_IDX_ROOT;
  bool found = false;

  readSector(&(node_fs_buffer.nodes[0]), FS_NODE_SECTOR_NUMBER);
  readSector(&(node_fs_buffer.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);
  readSector(&sector_fs_buffer, FS_SECTOR_SECTOR_NUMBER);

  while (i < 64 && !found) {
    if (strcmp(node_fs_buffer.nodes[i].name, metadata->node_name) && metadata->parent_index == node_fs_buffer.nodes[i].parent_node_index){
      foundEntryNode = i;
      found = true;
    } else {
      i++;
    }
  }

  if (!found){
    *return_code = FS_R_NODE_NOT_FOUND;
    return;
  }

  if (node_fs_buffer.nodes[i].sector_entry_index == FS_NODE_S_IDX_FOLDER) {
    metadata->filesize = 0;
    *return_code = FS_R_TYPE_IS_FOLDER;
    return;
  }
  memcpy(&sector_entry_buffer, &sector_fs_buffer.sector_list[node_fs_buffer.nodes[foundEntryNode].sector_entry_index], 16);
  for(j = 0; j < 16; j++){
    if(sector_entry_buffer.sector_numbers[j] != '\0'){
      readSector(metadata->buffer + (j*512), sector_entry_buffer.sector_numbers[j]);
    } else {
      break;
    }
  }
  metadata->filesize =  j * 512;
  *return_code = FS_SUCCESS;
}

void write(struct file_metadata *metadata, enum fs_retcode *return_code) {
  struct node_filesystem   node_fs_buffer;
  struct sector_filesystem sector_fs_buffer;
  struct map_filesystem    map_fs_buffer;
  int emptyNode = 0;
  int emptySector = 0;
  int i = 0;
  int j = 0;
  struct sector_entry buffer;
  bool found = false;
  bool isFile = false;
  unsigned int sizenow;

  readSector(&map_fs_buffer, FS_MAP_SECTOR_NUMBER);
  readSector(&(node_fs_buffer.nodes[0]), FS_NODE_SECTOR_NUMBER);
  readSector(&(node_fs_buffer.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);
  readSector(&sector_fs_buffer.sector_list, FS_SECTOR_SECTOR_NUMBER);

  while (i < 64 && !found) {
    if (strcmp(node_fs_buffer.nodes[i].name, metadata->node_name) && 
    metadata->parent_index == node_fs_buffer.nodes[i].parent_node_index){
      found = true;
      if(node_fs_buffer.nodes[i].sector_entry_index == FS_NODE_S_IDX_FOLDER) isFile = false;
      else isFile = true;
      break;
    }
    i++;
  }
  if (found) {
    if(isFile) *return_code = FS_W_FILE_ALREADY_EXIST;
    else *return_code = FS_W_FOLDER_ALREADY_EXISTS;
    return;
  }
  if (strlen(metadata->node_name) > 13) {
    *return_code = FS_W_NOT_ENOUGH_STORAGE;
    return;
  }

  for (j = 0; j < 64; j++) {
    if (node_fs_buffer.nodes[j].name[0] == '\0') {
      emptyNode = j;
      break;
    }
  }
  if (j == 64) {
    *return_code = FS_W_MAXIMUM_NODE_ENTRY;
    return;
  }

  if (metadata->parent_index != FS_NODE_P_IDX_ROOT && node_fs_buffer.nodes[metadata->parent_index].sector_entry_index != FS_NODE_S_IDX_FOLDER) {
    *return_code = FS_W_INVALID_FOLDER;
    return;
  }

  if (metadata->filesize > 8192) {
    *return_code = FS_W_NOT_ENOUGH_STORAGE;
    return;
  }
  sizenow = metadata->filesize;
  for (i = 0; i < 512; i++) {
    if (sizenow <= 0) {
      break;
    }
    if (!map_fs_buffer.is_filled[i]) {
      sizenow -= 512;
    }
  }
  if (sizenow > 0) {
    *return_code = FS_W_NOT_ENOUGH_STORAGE;
    return;
  }

  if(metadata->filesize > 0){
    for (i = 0; i < 32; i++) {
      if (sector_fs_buffer.sector_list[i].sector_numbers[0] == '\0') {
        emptySector = i;
        break;
      }
    }
    if (i == 32) {
      *return_code = FS_W_MAXIMUM_SECTOR_ENTRY;
      return;
    }
  }

  if (metadata->filesize == 0) {
    node_fs_buffer.nodes[emptyNode].sector_entry_index = FS_NODE_S_IDX_FOLDER;
  } 
  else {
    i = 0;
    for (j = 16; j < 256; j++) {
      if (map_fs_buffer.is_filled[j]) {
        continue;
      }
      map_fs_buffer.is_filled[j] = true;
      buffer.sector_numbers[i] = j;
      i++;
      if (metadata->filesize <= 512 * i) {
        break;
      }
    }
    for (j = 0; j < i; j++) {
      writeSector(metadata->buffer + j * 512, buffer.sector_numbers[j]);
    }
    memcpy(&sector_fs_buffer.sector_list[emptySector], &buffer, i);
    node_fs_buffer.nodes[emptyNode].sector_entry_index = emptySector;
    writeSector(&map_fs_buffer, FS_MAP_SECTOR_NUMBER);
    writeSector(&sector_fs_buffer.sector_list, FS_SECTOR_SECTOR_NUMBER);
  }
  strcpy(node_fs_buffer.nodes[emptyNode].name, metadata->node_name);
  node_fs_buffer.nodes[emptyNode].parent_node_index = metadata->parent_index;
  writeSector(&(node_fs_buffer.nodes[0]), FS_NODE_SECTOR_NUMBER);
  writeSector(&(node_fs_buffer.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);
  *return_code = FS_SUCCESS;
  return;
}