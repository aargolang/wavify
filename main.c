#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <malloc.h>

#define MAX_BYTES 0xFFFFFFFF - 44
#define NL printf("\n")

typedef struct wave_header
{
    // RIFF chunk descriptor
    char chunk_id[4];        // type of wave
    unsigned int chunk_size; // in bytes
    char chunk_format[4];    // file type header

    // WAVE fmt sub-chunk
    char fmt_id[4];                  // format chunk marker
    unsigned int fmt_size;           // length of format data
    unsigned short fmt_audio_format; // type of format. 1 is PCM
    unsigned short fmt_channels;     // number of channels
    unsigned int fmt_sample_rate;    // sampling rate
    unsigned int fmt_byte_rate;      // SamplingRate*BitsPerSample*Channels/8
    unsigned short fmt_block_align;  // Bitspersample*Channels/?
    unsigned short fmt_bps;          // bits per sample

    // WAVE data sub-chunk
    char data_id[4];        // data chunk header. marks beginning of data section
    unsigned int data_size; // size of the data section
} WH;

////////////////////////////////////////////////////////////////////////////////
int swap_32(int num)
{
    int swapped = ((num >> 24) & 0xff) |      // move byte 3 to byte 0
                  ((num << 8) & 0xff0000) |   // move byte 1 to byte 2
                  ((num >> 8) & 0xff00) |     // move byte 2 to byte 1
                  ((num << 24) & 0xff000000); // byte 0 to byte 3

    return swapped;
}

////////////////////////////////////////////////////////////////////////////////
void print_n(char *source, int n)
{
    // prints our n characters from a source
    char *cp = source;
    for (int i = 0; i < n; i++)
    {
        if (*cp >= ' ' && *cp <= '~')
        { // only print printable characters
            printf("%c", *cp);
        }
        else
        {
            printf("-");
        }
        cp++;
    }
}

//////////////////////////////////////////////////////////////////////////////
void pbh(void *source, unsigned int len)
{
    unsigned int i;
    for (i = 0; i < len; ++i)
    {
        printf("%02X ", ((unsigned char *)source)[i]);
    }
}

void print_wav_header(WH *w)
{
    printf("chunk::id: ");
    print_n(w->chunk_id, 4);
    NL;
    printf("chunk::size: %d\n", w->chunk_size);
    printf("chunk::format: ");
    print_n(w->chunk_format, 4);
    NL;
    printf("fmt::id: ");
    print_n(w->fmt_id, 4);
    NL;
    printf("fmt::len: %d\n", w->fmt_size);
    printf("fmt::audio format: %d\n", w->fmt_audio_format);
    printf("fmt::num channels: %d\n", w->fmt_channels);
    printf("fmt::sample rate: %d\n", w->fmt_sample_rate);
    printf("fmt::byte rate: %d\n", w->fmt_byte_rate);
    printf("fmt::bits per sample: %d\n", w->fmt_bps);
    printf("data::id: ");
    print_n(w->data_id, 4);
    NL;
    printf("data::size: %d\n", w->data_size);
}

////////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
    char *buff;
    FILE *fp;
    char in_file_name[256];
    char out_file_name[256];

    int bit_depth = 32;
    int channels = 1;
    int sampling_rate = 44100;

    if (argc != 3)
    {
        printf("usage: wavify <input_file> <output_file>");
    }

    // parse args
    strncpy(in_file_name, argv[1], 255);
    strncpy(out_file_name, argv[2], 255);
    out_file_name[255] = '\0';
    in_file_name[255] = '\0';

    // open file
    fp = fopen(in_file_name, "r");
    if (fp == NULL)
    {
        perror("not open file");
        return 1;
    }

    // verify file size
    fseek(fp, 0L, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    if (file_size >= MAX_BYTES)
    {
        printf("file can't be larger than 4 \'Gibibytes\'\n");
        fclose(fp);
        return 1;
    }

    // allocate memory
    buff = (char *)malloc(file_size + 44);

    if (fread(buff + 44, file_size, 1, fp) < 0)
    {
        perror("not gotten\n");
        fclose(fp);
        return 1;
    }
    fclose(fp);



    

    // create new file
    WH *fh = (WH *)buff;
    memcpy(fh->chunk_id, "RIFF", 4);
    fh->chunk_size = file_size + 44 - 8;
    memcpy(fh->chunk_format, "WAVE", 4);
    memcpy(fh->fmt_id, "fmt ", 4);
    fh->fmt_size = 16;
    fh->fmt_audio_format = 1;
    fh->fmt_channels = channels;
    fh->fmt_sample_rate = sampling_rate;
    fh->fmt_bps = bit_depth;
    fh->fmt_byte_rate = fh->fmt_channels * fh->fmt_sample_rate * fh->fmt_bps / 8;
    fh->fmt_block_align = fh->fmt_channels * fh->fmt_bps / 8;
    memcpy(fh->data_id, "data", 4);
    fh->data_size = file_size;

    // check on things
    // print_wav_header(fh);
    // NL;
    // pbh(buff, file_size + 44);
    // NL;

    
    // write to file

    // write_ptr = fopen("created_file.wav","wb");  // w for write, b for binary

    fp = fopen(out_file_name, "wb");

    if (fp == NULL)
    {
        printf("problem opening file to write\n");
        perror("error opening file to write");
        return 1;
    }

    if (fwrite(buff, file_size + 44, 1, fp) < 0)
    {
        perror("problem writing to file");
        return 1;
    }
    fclose(fp);

    return 0;
}

/* tried to print samples but wrong format 
  //char *cp = (char*)(byte_buff + sizeof(WH));
  char *cp = (char*)&byte_buff;
  cp = cp + 44;
  int *ip = (int*)cp;
  float *flp = (float*)&byte_buff;
  flp = flp + 11;

  printf("bytebuff ");
  pbh(byte_buff, 54); NL;
  printf("&byte_buff %x\n", &byte_buff);
  printf("flp %x\n", flp);

  // int *ip = &byte_buff;

  pbh(byte_buff, 4); NL;
  pbh(cp, 4); NL;
  pbh(ip, 4); NL;
  pbh(flp, 4); NL;

  printf("printing first 50 samples\n");
  for (int i = 0; i<20; i++)
  {
    printf("sample %d = %f\n", i, *flp);
    flp++;
  } */
