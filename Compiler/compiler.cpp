#include "compiler.h"

template<class T>
std::string t_to_string(T i)
{
    std::stringstream ss;
    std::string s;
    ss << i;
    s = ss.str();

    return s;
}

FILE* data_file_h;
FILE* data_file_c;

void compile_bitmap(string root_path, string bitmap_path)
{
    string bitmap_name = bitmap_path.substr(0, bitmap_path.size()-4);
    printf("compiling %s...\n", bitmap_name.c_str());

    FILE* bitmap = fopen((root_path+bitmap_path).c_str(), "rb");

    // bitmap header

    char bitmap_header[2];
    fread(bitmap_header, 1, 2, bitmap);
    unsigned int bitmap_size;
    fread(&bitmap_size, 4, 1, bitmap);
    unsigned int reserved;
    fread(&reserved, 4, 1, bitmap);
    unsigned int bitmap_offset;
    fread(&bitmap_offset, 4, 1, bitmap);

    // DIB header

    unsigned int dib_size;
    fread(&dib_size, 4, 1, bitmap);
    unsigned int bitmap_width;
    fread(&bitmap_width, 4, 1, bitmap);
    unsigned int bitmap_height;
    fread(&bitmap_height, 4, 1, bitmap);
    unsigned int reserved2;
    fread(&reserved2, 2, 1, bitmap);
    unsigned short bitmap_bpp;
    fread(&bitmap_bpp, 2, 1, bitmap);
    fread(&reserved2, 4, 1, bitmap);
    fread(&reserved2, 4, 1, bitmap);
    fread(&reserved2, 4, 1, bitmap);
    fread(&reserved2, 4, 1, bitmap);
    unsigned int bitmap_depth;
    fread(&bitmap_depth, 4, 1, bitmap);

    if (bitmap_width!=400 || bitmap_height!=300)
    {
        printf("INCORRECT SIZE\n");
        fclose(bitmap);
        return;
    }

    if (bitmap_bpp!=24)
    {
        printf("INCORRECT DEPTH / BPP\n");
        fclose(bitmap);
        return;
    }

    fprintf(data_file_h, "    extern const unsigned char %s[];\n", bitmap_name.c_str());

    // read image data

    unsigned char* image = (unsigned char*)malloc(bitmap_width*bitmap_height*3);

    for (int y=0 ; y<bitmap_height ; ++y)
    {
        fseek(bitmap, bitmap_offset+(bitmap_height-y-1)*bitmap_width*3, SEEK_SET);
        fread(image+y*bitmap_width*3, bitmap_width*3, 1, bitmap);
    }

    int size = bitmap_width*bitmap_height*3;

    // write

    fprintf(data_file_c, "const unsigned char %s[] = {", bitmap_name.c_str());

    for (int i=0 ; i<size ; ++i)
    {
        fprintf(data_file_c, "%u", image[i]);
        if (i<size-1)
            fprintf(data_file_c, ",");
    }

    free(image);

    fprintf(data_file_c, "};\n");

    fclose(bitmap);
}

int main(int argc, char* argv[])
{
    string root_path = argv[1];
    printf("compiling data...\n");

    data_file_h = fopen((root_path+"data.h").c_str(), "wt");
    fprintf(data_file_h, "#ifndef __DATA_H__\n");
    fprintf(data_file_h, "#define __DATA_H__\n\n");

    data_file_c = fopen((root_path+"data.c").c_str(), "wt");
    fprintf(data_file_c, "#include \"data.h\"\n\n");

    DIR* dp;
    struct dirent* entry;
    struct stat statbuf;

    if ((dp = opendir(root_path.c_str()))==NULL)
    {
        printf("Cannot open directory\n");
        return -1;
    }
    while ((entry = readdir(dp))!=NULL)
    {
        int len = strlen(entry->d_name);
        if (len>4 && strcmp(entry->d_name+len-4, ".bmp")==0)
            compile_bitmap(root_path, entry->d_name);
    }
    closedir(dp);

    fprintf(data_file_h, "\n#endif\n");
    fclose(data_file_h);
    return 0;
}

