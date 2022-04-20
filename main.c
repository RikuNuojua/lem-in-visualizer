#include <raylib.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#define BYTES_PER_READ 64
#define NAME_LENGTH 32
#define FPS 60
#define FRAMES_PER_TICK 60


void
crash(char* p)
{
    printf("%s\n", p);
    exit(1);
}

int
main()
{
    //read
    size_t text_size = 0;
    char* p_text = NULL;
    {
        size_t allocated = BYTES_PER_READ;
        p_text = malloc(allocated);
        size_t read_bytes;
        while(1)
        {
            read_bytes = read(0, p_text + text_size, BYTES_PER_READ);
            if(read_bytes == 0) break; //EOF
            text_size += read_bytes;
            if(text_size + BYTES_PER_READ > allocated)
            {
                allocated += BYTES_PER_READ;
                p_text = realloc(p_text, allocated);
            }
        }
        printf("%lu chars read\n", text_size);
    }


    //parse
    int* p_ant_x_ticks; // xtick-0(ant-0 ant-1 ant-2... ant-ant_count) xtick-1(..)... xtick-tick_count(...)
    int* p_ant_y_ticks; // tick_count grows with realloc
    size_t ant_count;
    size_t tick_count;
    size_t room_count;
    int start_index;
    int end_index;
    char* p_room_names; //use NAME_LENGTH
    int* p_room_xs;
    int* p_room_ys;
    {
        char* p = p_text;
        //ant count
        ant_count = 0;
        while(1)
        {
            if(*p == '#')
            {
               while(*p != '\n') p++;
               p++;
               continue;
            }
            while(*p >= '0' && *p <= '9')
            {
                ant_count = ant_count * 10;
                ant_count = ant_count + (*p - '0');
                p++;
            }
            if(*p == '\n') 
            {
                p++;
                break;
            }
            crash("Error: ant count");
        }

        //rooms
        room_count = 0;
        size_t room_allocated = 0;
        p_room_names = NULL;
        p_room_xs = NULL;
        p_room_ys = NULL;
        while(1)
        {
            assert(p[-1] == '\n');
            //#
            if(*p == '#')
            {
                if(p[1] == '#')
                {
                    if(0 == strncmp(p, "##start", 7)) start_index = room_count;
                    else if(0 == strncmp(p, "##end", 5)) end_index = room_count;
                    printf("start: %d\n", start_index);
                    printf("end: %d\n", end_index);
                }
                while(*p != '\n') p++;
                p++;
                continue;
            }

            //realloc
            if(room_allocated <= room_count+1)
            {
                room_allocated += 2; //todo times two
                p_room_names = realloc(p_room_names, room_allocated * NAME_LENGTH);
                p_room_xs = realloc(p_room_xs, room_allocated * sizeof(int));
                p_room_ys = realloc(p_room_ys, room_allocated * sizeof(int));
            }

            //room name
            int i = 0;
            char* start_of_line = p;
            while(1)
            {
                if(*p == '-') //end of rooms, start of links.
                {
                    p = start_of_line;
                    break;
                }
                if(*p == '\n') crash("Error: bad room name");
                if(i+1 >= NAME_LENGTH) crash("Error: too long room name");
                if(*p == ' ') //this allows blank names
                {
                    p_room_names[room_count * NAME_LENGTH + i + 1] = '\0';
                    printf("Room %lu: %s\n", room_count, p_room_names + (room_count * NAME_LENGTH));
                    p++;
                    break;
                }
                p_room_names[room_count * NAME_LENGTH + i] = *p;
                p++;
                i++;
            }
            if(p == start_of_line) break; //end of rooms start of links

            //room x
            if(*p >= '0' && *p <= '9')
            {
                int x = 0;
                while(*p >= '0' && *p <= '9')
                {
                    x *= 10;
                    x += *p - '0';
                    p++;
                }
                p_room_xs[room_count] = x;
            }
            else crash("Error: room x");
            if(*p != ' ') crash("Error: room xy seperator is not space");
            p++;

            //room y
            if(*p >= '0' && *p <= '9')
            {
                int y = 0;
                while(*p >= '0' && *p <= '9')
                {
                    y *= 10;
                    y += *p - '0';
                    p++;
                }
                p_room_ys[room_count] = y;
            }
            else crash("Error: room y");
            if(*p != '\n') crash("Error: room doesnt end in '\\n'");
            room_count++;
            p++;
        }
        //rooms

        //links
        //TODO: save links and draw em!
        {
            while(!(*p == 'L' && p[-1] == '\n')) p++;
        }
        //links
        
        //ant ticks
        size_t tick_allocation = 8;
        tick_count = 1;
        p_ant_x_ticks = malloc(sizeof(int) * ant_count * tick_allocation);
        p_ant_y_ticks = malloc(sizeof(int) * ant_count * tick_allocation);
        //starting position
        for(int ant = 0; ant < ant_count; ant++)
        {
            p_ant_x_ticks[ant] = p_room_xs[start_index];
            p_ant_y_ticks[ant] = p_room_ys[start_index];
        }
        {
            size_t ant;
            int nl_detected = 1;
            while(1)
            {
                if(*p != 'L')
                {
                    if(*p == '\n' || *p == 0) break;
                    printf("\n%c", *p);
                    crash("-fuck\n");
                }
                p++;
                if(tick_allocation <= tick_count + 1)
                {
                    tick_allocation += 2;
                    p_ant_x_ticks = realloc(p_ant_x_ticks, sizeof(int) * ant_count * tick_allocation);
                    p_ant_y_ticks = realloc(p_ant_y_ticks, sizeof(int) * ant_count * tick_allocation);
                }
                //set new tick positions same as the tick before
                for(int i = 0; nl_detected && (i < ant_count); i++)
                {
                    size_t offset = (ant_count * tick_count) + i;
                    p_ant_x_ticks[offset] = p_ant_x_ticks[offset - ant_count];
                    p_ant_y_ticks[offset] = p_ant_y_ticks[offset - ant_count];
                }
                if(*p >= '0' && *p <= '9')
                {
                    ant = 0;
                    while(*p >= '0' && *p <= '9')
                    {
                        ant *= 10;
                        ant += *p - '0';
                        p++;
                    }
                    ant--; //input ant numbers start with 1?
                }
                else crash("Error: ant number");
                if(*p != '-') crash("Error: ant seperator");
                p++;

                //find mathing name by index
                nl_detected = 0;

                int match = -1;
                int n = 0;
                int i = 0;
                while(i < room_count)
                {
                    if(p_room_names[i * NAME_LENGTH + n] == '\0')
                    {
                        if(p[n] == '\n')
                        {
                            match = i;
                            nl_detected = 1;
                            //printf("Newline\n");
                            p += n;
                            break;
                        }
                        else if(p[n] == ' ')
                        {
                            match = i;
                            //printf("Space\n");
                            p += n;
                            break;
                        }
                    }
                    if(p_room_names[i * NAME_LENGTH + n] != p[n])
                    {
                        printf("Mismatch (%d): '%s', '%s'\n", n, p_room_names + (i * NAME_LENGTH), p);
                        n = 0;
                        i++;
                        continue;
                    }
                    else 
                    {
                        n++;
                        continue;
                    }
                    assert(0);
                }
                if(match == -1) crash("Error: instruction bad name");
                p_ant_x_ticks[tick_count * ant_count + ant] = p_room_xs[match];
                p_ant_y_ticks[tick_count * ant_count + ant] = p_room_ys[match];
                tick_count += nl_detected;
                if(*p != ' ' && *p != '\n') crash("Error: expected \\n or space");
                p++;
            }
        }
        //ant ticks
        printf("%lu animation ticks!\n", tick_count);
    }
    //raylib window creation and drawing
    {
        int win_x;
        int win_y;
        int x;
        int y;
        int next_x;
        int next_y;
        float lerp;
        size_t frame = 0;
        float zoom = 32.0;

        SetConfigFlags(FLAG_WINDOW_RESIZABLE);
        InitWindow(0, 0, "raylib [core] example - basic window");
        SetTargetFPS(FPS);

        while (!WindowShouldClose())
        {
            win_x = GetScreenWidth();
            win_y = GetScreenHeight();
            int offset_x = win_x/2;
            int offset_y = win_y/2;
            int n;
            BeginDrawing();
            {
                ClearBackground(RAYWHITE);
                DrawFPS(0, 0);
                //draw rooms
                Color color;
                for(int i = 0; i < room_count; i++)
                {
                    x = offset_x - 20 + (int)(zoom * (float)p_room_xs[i]);
                    y = offset_y - 20 + (int)(zoom * (float)p_room_ys[i]);
                    color = GRAY;
                    if(i == start_index) color = DARKBLUE;
                    if(i == end_index) color = MAROON;
                    DrawRectangle(x, y, 40, 40, color);
                }
                lerp = ((float)(frame % ((size_t)FRAMES_PER_TICK))) / (float)FRAMES_PER_TICK;
                size_t anim_tick = (frame % ((size_t)tick_count * (size_t)FRAMES_PER_TICK)) / ((size_t)FRAMES_PER_TICK);
                DrawCircle(10, 10, 2.0f + 25.0f*(float)anim_tick, BLACK);
                for(size_t i = 0; i < ant_count; i++)
                {
                    n = anim_tick*ant_count + i;
                    x = offset_x + (int)(zoom * (float)p_ant_x_ticks[n]);
                    y = offset_y + (int)(zoom * (float)p_ant_y_ticks[n]);
                    //dont interp after last frame
                    n = (1+anim_tick) * ant_count + i;
                    if(anim_tick < tick_count-1) next_x = offset_x + (int)(zoom * (float)p_ant_x_ticks[n]);
                    else next_x = x;
                    if(anim_tick < tick_count-1) next_y = offset_y + (int)(zoom * (float)p_ant_y_ticks[n]);
                    else next_y = y;

                    x = x + (int)(((float)next_x - (float)x) * lerp);
                    y = y + (int)(((float)next_y - (float)y) * lerp);
                    DrawCircle(x, y, 10.0f, BLACK);
                }
            }
            EndDrawing();
            frame++;
        }
        // window closed
    }
    return 0;
}
