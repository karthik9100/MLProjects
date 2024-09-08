#include<ncurses.h>
#include<string.h>
#include<stdlib.h>
#include<X11/Xlib.h>
char buffer[2048][2048];
int blen=0;
static Display *display;
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

    //this is for creating x11 window

    display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "Unable to open X Display\n");
        exit(1);
    }

    int screen = DefaultScreen(display);
    Window window = XCreateSimpleWindow(display, RootWindow(display, screen), 0, 0, 640, 480, 1,
    BlackPixel(display, screen), WhitePixel(display, screen));
    XMapWindow(display, window);
    // XFlush(display);

    //end of x11 window code
    int ch;
    char temp[100];
    int row=0,col=0;
    int maxrow,maxcol;
    getmaxyx(stdscr,maxrow,maxcol);
    if(argc!=2)
    {
        printf("please enter file name");
        exit(1);
    }
    char filename[100];
    strcpy(filename,argv[1]);
    printf("%s",filename);
    FILE *ptr = fopen(filename,"a+");
    if(ptr==NULL)
    {
        printf("unable to open file");
        exit(1);
    }
    initscr();
    raw();
    keypad(stdscr,true);
    refresh();
    noecho();
    // printw("enter any word to see it in BOLD\n");
    while (fgets(temp,sizeof(temp),ptr) != NULL) {
         
            addstr(temp); // Display the character
            strcat(buffer[row],temp);
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
            getyx(stdscr,row,col);
            for(int i=0;i<=row;i++)
            {
                // addstr(buffer[i]);
                fputs(buffer[i],ptr);
            }
            
            continue;
        }
        if(ToggleCurser(ch))
            continue;
        if(ch==KEY_F(1))
            printw("F1 is pressed");
        else
        {
            // printw("the key pressed is\n");
            // attron(A_BOLD);
            getyx(stdscr,row,col);
            // printw("%d %d...",row,col);
            buffer[row][col] = ch;
            printw("%c",ch);
            // buffer[blen++] = ch;
            
            // // printw("%c",24);
            // attroff(A_BOLD);

        }
       
        
        
    }
    // fclose(ptr);
    endwin();

    XCloseDisplay(display);
}