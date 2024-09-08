#include<ncurses.h>
#include<string.h>
#include<stdlib.h>
char buffer[2048][2048];
int blen=0;
int ToggleCurser(int ch)
{
    int row,col,x=0,y=0,maxrow,maxcol;
    getyx(stdscr,row,col);
    getmaxyx(stdscr,maxrow,maxcol);
    
    if(ch==259)
        x=-1;
    else if(ch==258 )
        x=1;
    else if(ch==261)
        y=1;
    else if(ch==260)
        y=-1;
    
    if(ch==330 ||ch==263)
    {
        
        y=-1;
        move(row,col-1);
        // if(blen>0)
        // buffer[--blen]='\0';
        buffer[row][col]=' ';
        printw(" ");

    }
    move(row+x,col+y);
    
    if(x!=0||y!=0)
        return 1;
    return 0;
    // if(col+y==0)
    //     move(row-1+x,maxcol);

}
void main(int argc,char **argv )
{
    int ch;
    char temp[100];
    int row=0,col=0;
    int maxrow,maxcol;
    int characters_count=0;
    getmaxyx(stdscr,maxrow,maxcol);
    int modified_rows[100];
    for(int i=0;i<100;i++)
        modified_rows[i]=0;
    if(argc!=2)
    {
        printf("please enter file name");
        exit(1);
    }
    char filename[100];
    strcpy(filename,argv[1]);
    printf("%s",filename);
    FILE *ptr = fopen(filename,"r+");
    if(ptr==NULL)
    {
        ptr = fopen(filename,"w+");
        if(ptr==NULL)
        {
            printf("unable to open file");
            exit(1);
        }
            
    }
    initscr();
    raw();
    keypad(stdscr,true);
    refresh();
    noecho();
    // printw("enter any word to see it in BOLD\n");
    while (fgets(temp,sizeof(temp),ptr) != NULL) {
            strcat(buffer[row],temp);         
            addstr(temp); // Display the character
            
            row++;
            move(row,col);
        
    }
    getyx(stdscr,row,col);
    while(1)
    {
        ch = getch();
        if(ch==27 || ch==24)
            break;
        
        if(ch==19)
        { 
            // printw("cntrl s");
            
            fseek(ptr,0,SEEK_SET);
            // ptr.seek(0);
            getyx(stdscr,row,col);
            for(int i=0;i<=row;i++)
            {
                // addstr(buffer[i]);
                fprintf(ptr,"%s",buffer[i]);
            }
            
            continue;
        }
        if(ToggleCurser(ch))
            continue;
        if(ch==KEY_F(1))
            printw("F1 is pressed");
        else //if((ch>=65 && ch<=90) || (ch>=48 && ch<=57) ||(ch>=97 && ch<=122) || ch==32 || ch==10)
        {
            // printw("the key pressed is\n");
            // attron(A_BOLD);
            getyx(stdscr,row,col);
            // printw("%d %d...",row,col);
            buffer[row][col] = ch;
            modified_rows[row]=1;
            printw("%c",ch);
            if(ch!=10)
            characters_count+=1;
            // buffer[blen++] = ch;
            
            // // printw("%c",24);
            // attroff(A_BOLD);

        }
       
        
        
    }
    int rows_modified_count = 0;
    for(int i=0;i<100;i++)
    {
        if(modified_rows[i])
        {
            rows_modified_count++;
        }
    }
    printf("Number of characters affected  %d   Number of rows affected %d   ",characters_count,rows_modified_count);
    // getch();
    fclose(ptr);
    endwin();
}