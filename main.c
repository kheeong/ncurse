#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <wchar.h>

int score = 0;                   // score
int level = 1;                   // level
int grid_x = 10;                 // number of grid in x axis
int grid_y = 6;                  // number of grid in y axis
int number_of_human_in_grid = 3; // number of human in the initial grid
int speed = 500;                 // current speed
int max_speed = 100;             // max speed - the lower the faster
int max_danger_zone = 10;        // max number of danger zone
int score_to_next_level = 0;     // how much score is needed to reach next level
int lives = 3;                   // number of lives
int c = 0;                       // test how many times the loop run - debug only
int immunity_time = 1;           // how long the immunity last

// the number of character in the human, danger_zone and robot must be the same
char human[] = " o<-<  ";       // human character
char danger_zone[] = "XXXXXXX"; // danger zone character
char robot[100] = ".[ ''].";

char last_key_press_up_down[100] = "up";               // last key press up or down - for robot eye direction
char last_key_press_left_right[100] = "right";         // last key press left or right - for robot eye direction
char last_key_press[100] = "right";                    // last key press - for determining the direction of the robot
char leader_board_text_file[100] = "leader_board.txt"; // leader board text file
bool developer_mode = true;                            // developer mode - show more detail
bool immunity = false;                                 // the immunity flag - if true the robot will not die when hit the danger zone
bool game_over_flag = false;                           // game over flag - if true the game will end
time_t immunity_start_time;                            // intialize the immunity start time

void rectangle(int y1, int x1, int y2, int x2)
{
    // draw rectangle boarder - replace box() because box() is not working
    mvhline(y1, x1, 0, x2 - x1);
    mvhline(y2, x1, 0, x2 - x1);
    mvvline(y1, x1, 0, y2 - y1);
    mvvline(y1, x2, 0, y2 - y1);
    mvaddch(y1, x1, ACS_ULCORNER);
    mvaddch(y2, x1, ACS_LLCORNER);
    mvaddch(y1, x2, ACS_URCORNER);
    mvaddch(y2, x2, ACS_LRCORNER);
}

const char *robot_direction(char *up_down, char *left_right)
{
    // return the robot eye direction
    char *direction;
    if (strcmp(up_down, "up") == 0)
    {
        if (strcmp(left_right, "left") == 0)
        {
            direction = ".['' ].";
        }
        else if (strcmp(left_right, "right") == 0)
        {
            direction = ".[ ''].";
        }
    }
    else if (strcmp(up_down, "down") == 0)
    {
        if (strcmp(left_right, "left") == 0)
        {
            direction = ".[.. ].";
        }
        else if (strcmp(left_right, "right") == 0)
        {
            direction = ".[ ..].";
        }
    }
    return direction;
}

int show_detail()
{
    // show the score level and lives
    // also show the last key press, speed, count and game over flag if developer mode is on
    move(2 + grid_y * 2, 0);
    clrtoeol();
    printw("Score: %d   Level: %d   Lives: %d \n", score, level, lives);
    if (developer_mode == true)
    {
        clrtoeol();
        printw("Last key press: %s  Speed: %d   count: %d   Game over flag: %s", last_key_press, speed, c, game_over_flag ? "true" : "false");
    }
    move(2 + grid_y * 2 + 1, 0); // move the cursor to the bottom of the screen
}

int print_robot_from_grid_coordinate(int x, int y, WINDOW *win)
{
    char robot[] = ".[ ''].";     // intialize the robot
    if (x > grid_x || y > grid_y) // check if the coordinate is in the grid
    {
        return 0;
    }
    strcpy(robot, robot_direction(last_key_press_up_down, last_key_press_left_right)); // get the robot charcter with the eye direction
    int actual_x = 1 + (x - 1) * (strlen(robot) + 1);                                  // calculate the actual x coordinate
    int actual_y = 1 + (y - 1) * 2;                                                    // calculate the actual y coordinate
    move(actual_y, actual_x);                                                          // move the cursor to the actual coordinate
    if (game_over_flag == true)
    {
        // if game over flag is true show the robot as dead
        addstr(".[x x].");
        move(2 + grid_y * 2 + 1, 0);
        return 1;
    }
    if (immunity == true)
    {
        // if immunity is true turn the robot red to show it is immune
        attron(COLOR_PAIR(1));
        addstr(robot);
        move(2 + grid_y * 2 + 1, 0);
        attroff(COLOR_PAIR(1));
        return 1;
    }
    else
    {
        // if immunity is false show the robot as normal
        addstr(robot);
        move(2 + grid_y * 2 + 1, 0);
    }
    return 1;
}

int clear_robot_from_grid_coordinate(int x, int y, WINDOW *win)
{
    // remove the robot from the grid
    // check if the coordinate is in the grid
    if (x > grid_x || y > grid_y)
    {
        return 0;
    }
    // calculate the actual coordinate
    int actual_x = 1 + (x - 1) * (strlen(robot) + 1);
    int actual_y = 1 + (y - 1) * 2;
    // move the cursor to the actual coordinate
    move(actual_y, actual_x);
    // replace the robot with space
    for (int i = 0; i < strlen(robot); i++)
    {
        addstr(" ");
    }
    return 1;
}

int print_grid(WINDOW *win)
{
    // print the grid
    int robot_size = strlen(robot);
    // print the vertical line
    for (int i = 0; i < grid_x; i++)
    {
        mvvline(1, robot_size + 1 + robot_size * i + i, ACS_VLINE, 1 * grid_y + grid_y - 1);
    }
    // print the horizontal line
    for (int i = 0; i < grid_y * 2; i = i + 2)
    {
        mvhline(2 + i, 1, ACS_HLINE, robot_size * grid_x + grid_x - 1);
    }
}

int place_human(int x, int y, WINDOW *win)
{
    // place human in the grid
    // check if the coordinate is in the grid
    if (x > grid_x || y > grid_y)
    {
        return 0;
    }
    // calculate the actual coordinate
    int actual_x = 1 + (x - 1) * (strlen(human) + 1);
    int actual_y = 1 + (y - 1) * 2;
    // move the cursor to the actual coordinate
    move(actual_y, actual_x);
    // print the human
    addstr(human);
    // move the cursor to the bottom of the screen
    move(2 + grid_y * 2 + 1, 0);
}

int place_danger_zone(int x, int y, WINDOW *win)
{
    // place danger zone in the grid
    // check if the coordinate is in the grid
    if (x > grid_x || y > grid_y)
    {
        return 0;
    }
    // calculate the actual coordinate
    int actual_x = 1 + (x - 1) * (strlen(danger_zone) + 1);
    int actual_y = 1 + (y - 1) * 2;
    // move the cursor to the actual coordinate
    move(actual_y, actual_x);
    // print the danger zone
    addstr(danger_zone);
    // move the cursor to the bottom of the screen
    move(2 + grid_y * 2 + 1, 0);
}

int check_if_got_human(int robot_x, int robot_y, int *human_x, int *human_y, int number_of_human, int *danger_zone_x, int *danger_zone_y, int *number_of_danger_zone, WINDOW *win)
{
    // check if the robot got human
    // loop through the human coordinate array to check if the robot location is same as a human
    for (int i = 0; i < number_of_human; i++)
    {
        if (robot_x == *(human_x + i) && robot_y == *(human_y + i))
        {
            // if the robot got human increase the score and remove the human
            score++;
            // reduce the total number of human in the grid by 1
            number_of_human_in_grid--;
            // generate 2 danger zone
            // check if the number of danger zone is less than the max danger zone
            // if it is less than the max danger zone generate 2 danger zone
            // else do nothing
            if (*number_of_danger_zone < max_danger_zone)
            {
                // do the loop twice to generate 2 danger zone
                for (int z = 0; z < 2; z++)
                {
                    // randomly generate the danger zone coordinate
                    *(danger_zone_x + *number_of_danger_zone) = rand() % grid_x + 1;
                    *(danger_zone_y + *number_of_danger_zone) = rand() % grid_y + 1;
                    bool is_danger_zone_same_as_human_or_robot = false;
                    // check if the danger zone is same as human or robot
                    // if it is same as human or robot generate another danger zone
                    // else proceed to the next step
                    while (1)
                    {
                        is_danger_zone_same_as_human_or_robot = false;
                        for (int j = 0; j < number_of_human; j++)
                        {
                            if (*(danger_zone_x + *number_of_danger_zone) == *(human_x + j) && *(danger_zone_y + *number_of_danger_zone) == *(human_y + j) || *(danger_zone_x + *number_of_danger_zone) == robot_x && *(danger_zone_y + *number_of_danger_zone) == robot_y)
                            {
                                *(danger_zone_x + *number_of_danger_zone) = rand() % grid_x + 1;
                                *(danger_zone_y + *number_of_danger_zone) = rand() % grid_y + 1;
                                is_danger_zone_same_as_human_or_robot = true;
                            }
                        }
                        if (is_danger_zone_same_as_human_or_robot == false)
                        {
                            break;
                        }
                    }
                    // incerese the number of danger zone by 1
                    *number_of_danger_zone = *number_of_danger_zone + 1;
                }
            }
            show_detail();
            // remove the human from the array
            *(human_x + i) = -1;
            *(human_y + i) = -1;
            // check if there is any human left in the grid
            // if there is none, generate a new human
            if (number_of_human_in_grid == 0)
            {
                // check if the score to next level is 0
                // if it is 0 increase the level by 1 and reset the score to next level to 5
                // else decrease the score to next level by 1
                if (score_to_next_level == 0)
                {
                    score_to_next_level = 5;
                    level++;
                }
                else
                {
                    score_to_next_level--;
                }
                // generate a new human
                number_of_human_in_grid = 1;
                *human_x = rand() % grid_x + 1;
                *human_y = rand() % grid_y + 1;
                // check if human is same as robot
                // if it is same as robot generate another human
                bool is_human_same_as_robot = false;
                while (1)
                {
                    is_human_same_as_robot = false;
                    if (*human_x == robot_x && *human_y == robot_y)
                    {
                        *human_x = rand() % grid_x + 1;
                        *human_y = rand() % grid_y + 1;
                        is_human_same_as_robot = true;
                    }
                    if (is_human_same_as_robot == false)
                    {
                        break;
                    }
                }
                bool is_human_same_as_danger_zone = false;
                // check if human is same as danger zone
                // if it is same as danger zone generate another human
                while (1)
                {
                    is_human_same_as_danger_zone = false;
                    for (int j = 0; j < *number_of_danger_zone; j++)
                    {
                        if (*human_x == *(danger_zone_x + j) && *human_y == *(danger_zone_y + j))
                        {
                            *human_x = rand() % grid_x + 1;
                            *human_y = rand() % grid_y + 1;
                            is_human_same_as_danger_zone = true;
                        }
                    }
                    if (is_human_same_as_danger_zone == false)
                    {
                        break;
                    }
                }
                // place the human
                place_human(*human_x, *human_y, win);
            }
            return 1;
        }
    }
    return 0;
}

int leader_board(char *name, int score)
{
    // leader board
    int lines = 0; // the number of lines in the text file
    // the struct for the player in the leader board
    struct player
    {
        char name[20];
        int score;
    };
    // read the number of lines in the text file
    FILE *read_line = fopen(leader_board_text_file, "r");
    if (read_line == NULL)
    {
        printf("no such file.\n");
        return 0;
    }
    int file_buffer;
    while (!feof(read_line))
    {
        file_buffer = fgetc(read_line);
        if (file_buffer == '\n')
        {
            lines++;
        }
    }
    fclose(read_line);
    // read the player name and score from the text file
    FILE *leader_board_ranking = fopen(leader_board_text_file, "r+");
    if (leader_board_ranking == NULL)
    {
        printf("no such file.\n");
        return 0;
    }
    // store the player name and score in the struct
    struct player player_ranking[lines + 1];
    int i = 0;
    char buffer[100]; // temporarily store the name and score
    // skip the first line as it is the header
    fscanf(leader_board_ranking, "%s", buffer);
    fscanf(leader_board_ranking, "%s", buffer);
    // store the player name and score in the struct
    while (fscanf(leader_board_ranking, "%s", buffer) == 1)
    {
        strcpy(player_ranking[i].name, buffer);
        fscanf(leader_board_ranking, "%s", buffer);
        player_ranking[i].score = atoi(buffer);
        i++;
    }
    // store the new player name and score in the struct
    strcpy(player_ranking[lines - 1].name, name);
    player_ranking[lines - 1].score = score;
    // sort the player by score
    for (int j = 0; j < lines; j++)
    {
        for (int k = 0; k < lines - 1; k++)
        {
            if (player_ranking[k].score < player_ranking[k + 1].score)
            {
                struct player temp = player_ranking[k];
                player_ranking[k] = player_ranking[k + 1];
                player_ranking[k + 1] = temp;
            }
        }
    }
    fclose(leader_board_ranking);
    // write the sorted player name and score to the text file
    FILE *leader_board_ranking_write = fopen(leader_board_text_file, "w");
    if (leader_board_ranking_write == NULL)
    {
        printf("no such file.\n");
        return 0;
    }
    // write the header to the text file
    fprintf(leader_board_ranking_write, "%s %s\n", "NAME", "SCORE");
    // write the player name and score to the text file
    for (int j = 0; j < lines; j++)
    {
        fprintf(leader_board_ranking_write, "%s %d\n", player_ranking[j].name, player_ranking[j].score);
    }
    fclose(leader_board_ranking_write);
    // find the player rank
    int player_rank = 0;
    for (int j = 0; j < lines; j++)
    {
        if (strcmp(player_ranking[j].name, name) == 0)
        {
            player_rank = j + 1;
        }
    }
    // print the leader board
    if (lines > 5)
    {
        printf("Top 5 players:\n");
        for (int j = 0; j < 5; j++)
        {
            printf("%d.%s %d \n", j + 1, player_ranking[j].name, player_ranking[j].score);
        }
        printf("\n");
        printf("Your rank: %d\n", player_rank);
    }
    else
    {
        printf("Top %d players:\n", lines);
        for (int j = 0; j < lines; j++)
        {
            printf("%d.%s %d \n", j + 1, player_ranking[j].name, player_ranking[j].score);
        }
        printf("\n");
        printf("Your rank: %d\n", player_rank);
    }
    // ps some times this part work really weirdly
    // there might be some random character in the name
    // test a few more times as most of the time it work
    // yes same code different result idk why
}

int game_over()
{
    // game over screen
    // I don't use ncurses here because ehco(), nocbreak(), wscanw(), getstr() is not working
    printf("Game over\n");
    printf("Score: %d\n", score);
    printf("Level: %d\n", level);
    printf("Enter your name: ");
    char name[20] = "";
    scanf("%s", name);
    printf("\n");
    /* Assuming that test.txt has content
        in below format
        NAME SCORE
        abc 12
        bef 25
        cce 65 */
    leader_board(name, score);
    return 0;
}

int check_if_danger_zone(int robot_x, int robot_y, int *danger_zone_x, int *danger_zone_y, int number_of_danger_zone, WINDOW *win)
{
    // check if the robot hit the danger zone
    // loop through the danger zone coordinate array to check if the robot location is same as a danger zone
    for (int i = 0; i < number_of_danger_zone; i++)
    {
        if (robot_x == *(danger_zone_x + i) && robot_y == *(danger_zone_y + i))
        {
            // check if the robot is immune
            // if it is immune do nothing
            // else reduce the lives by 1 and set the immunity flag to true and set the immunity start time
            if (immunity == false)
            {
                lives--;
                immunity = true;
                immunity_start_time = time(NULL);
            }
            show_detail();
            return 1;
        }
    }
    return 0;
}

int main()
{
    int robot_size = strlen(robot);
    int robot_x = 1;                                                                       // initial robot x coordinate
    int robot_y = 1;                                                                       // initial robot y coordinate
    int ch;                                                                                // buffer for the key press
    int number_of_human = 3;                                                               // number of human
    int danger_zone_number = 0;                                                            // number of danger zone
    int danger_zone_x[grid_x * grid_y];                                                    // initial danger zone x coordinate
    int danger_zone_y[grid_x * grid_y];                                                    // initial danger zone y coordinate
    int human_x[number_of_human];                                                          // initial human x coordinate
    int human_y[number_of_human];                                                          // initial human y coordinate
    time_t t;                                                                              // for random number
    srand((unsigned)time(&t));                                                             // assgin seed for random number
    initscr();                                                                             // Start curses mode
    keypad(stdscr, TRUE);                                                                  // Enable arrow keys
    cbreak();                                                                              // Don't echo() while we do getch
    curs_set(0);                                                                           // Don't display a cursor
    start_color();                                                                         // Start color
    raw();                                                                                 // disable line buffering
    WINDOW *win = newwin(1 * grid_y + grid_y + 1, robot_size * grid_x + grid_x + 1, 0, 0); // create a new window
    nodelay(win, TRUE);                                                                    // disable delay
    timeout(0);                                                                            // set the delay to 0
    print_grid(win);                                                                       // print the grid
    // box(win, 0, 0); this is not working idk why
    rectangle(0, 0, 1 * grid_y + grid_y, robot_size * grid_x + grid_x);                    // draw rectangle boarder
    wrefresh(win);                                                                         // refresh the window
    refresh();
    // getch();
    init_pair(1, COLOR_RED, COLOR_BLACK); // set the color pair for robot immune
    // place initial human
    for (int i = 0; i < number_of_human; i++)
    {
        human_x[i] = rand() % grid_x + 1;
        human_y[i] = rand() % grid_y + 1;
        // check if human is same as other human
        for (int j = 0; j < i; j++)
        {
            if (human_x[i] == human_x[j] && human_y[i] == human_y[j])
            {
                human_x[i] = rand() % grid_x + 1;
                human_y[i] = rand() % grid_y + 1;
                j = 0;
            }
        }
        // check if human location is same as robot
        // if it is same as robot generate another human
        bool is_human_same_as_robot = false;
        while (1)
        {
            is_human_same_as_robot = false;
            if (human_x[i] == robot_x && human_y[i] == robot_y)
            {
                human_x[i] = rand() % grid_x + 1;
                human_y[i] = rand() % grid_y + 1;
                is_human_same_as_robot = true;
            }
            if (is_human_same_as_robot == false)
            {
                break;
            }
        }
        // place the human
        place_human(human_x[i], human_y[i], win);
    }
    // print the intial robotg
    print_robot_from_grid_coordinate(robot_x, robot_y, win);
    show_detail();
    while (1)
    {
        // check if the robot is dead
        if (lives != 0)
        {
            // if the robot is not dead print the danger zone
            for (int i = 0; i < danger_zone_number; i++)
            {
                // check if the danger zone is same as the robot
                // if it is same as the robot do nothing
                if (robot_x != danger_zone_x[i] || robot_y != danger_zone_y[i])
                {
                    place_danger_zone(danger_zone_x[i], danger_zone_y[i], win);
                }
            }
        }
        wrefresh(win);
        c++;
        show_detail();
        // get the key press
        ch = getch();
        // assign the key press to the last key press variables
        // if esc or q is pressed end the game
        switch (ch)
        {
        case KEY_LEFT:
            strcpy(last_key_press_left_right, "left");
            strcpy(last_key_press, "left");
            break;
        case KEY_RIGHT:
            strcpy(last_key_press_left_right, "right");
            strcpy(last_key_press, "right");
            break;
        case KEY_UP:
            strcpy(last_key_press_up_down, "up");
            strcpy(last_key_press, "up");
            break;
        case KEY_DOWN:
            strcpy(last_key_press_up_down, "down");
            strcpy(last_key_press, "down");
            break;
        case 27:
            endwin();
            return 0;
            break;
        case 'q':
            endwin();
            return 0;
            break;
        default:
            break;
        }
        // move the robot
        if (strcmp(last_key_press, "right") == 0)
        {
            // check if the robot is in the grid
            if (robot_x < grid_x)
            {
                // if the robot is in the grid move the robot to the right
                clear_robot_from_grid_coordinate(robot_x, robot_y, win);
                robot_x++;
                print_robot_from_grid_coordinate(robot_x, robot_y, win);
                // check if the robot got human
                // else check if the robot hit the danger zone
                if (check_if_got_human(robot_x, robot_y, human_x, human_y, number_of_human, danger_zone_x, danger_zone_y, &danger_zone_number, win) == 1)
                {
                    continue;
                }
                else
                {
                    check_if_danger_zone(robot_x, robot_y, danger_zone_x, danger_zone_y, danger_zone_number, win);
                }
                wrefresh(win);
            }
            else
            {
                // if the robot is not in the grid print the robot at the same location
                // which mean the robot hit the boarder
                // check if the robot is immune
                // if it is immune do nothing
                // else reduce the lives by 1 and set the immunity flag to true and set the immunity start time
                print_robot_from_grid_coordinate(robot_x, robot_y, win);
                if (immunity == false)
                {
                    lives--;
                    immunity = true;
                    immunity_start_time = time(NULL);
                }
            }
        }
        // same as above but for other direction
        else if (strcmp(last_key_press, "left") == 0)
        {
            if (robot_x > 1)
            {
                clear_robot_from_grid_coordinate(robot_x, robot_y, win);
                robot_x--;
                print_robot_from_grid_coordinate(robot_x, robot_y, win);
                if (check_if_got_human(robot_x, robot_y, human_x, human_y, number_of_human, danger_zone_x, danger_zone_y, &danger_zone_number, win) == 1)
                {
                    continue;
                }
                else
                {
                    check_if_danger_zone(robot_x, robot_y, danger_zone_x, danger_zone_y, danger_zone_number, win);
                }
                wrefresh(win);
            }
            else
            {
                print_robot_from_grid_coordinate(robot_x, robot_y, win);
                if (immunity == false)
                {
                    lives--;
                    immunity = true;
                    immunity_start_time = time(NULL);
                }
            }
        }
        else if (strcmp(last_key_press, "up") == 0)
        {
            if (robot_y > 1)
            {
                clear_robot_from_grid_coordinate(robot_x, robot_y, win);
                robot_y--;
                print_robot_from_grid_coordinate(robot_x, robot_y, win);
                if (check_if_got_human(robot_x, robot_y, human_x, human_y, number_of_human, danger_zone_x, danger_zone_y, &danger_zone_number, win) == 1)
                {
                    continue;
                }
                else
                {
                    check_if_danger_zone(robot_x, robot_y, danger_zone_x, danger_zone_y, danger_zone_number, win);
                }
                wrefresh(win);
            }
            else
            {
                print_robot_from_grid_coordinate(robot_x, robot_y, win);
                if (immunity == false)
                {
                    lives--;
                    immunity = true;
                    immunity_start_time = time(NULL);
                }
            }
        }
        else if (strcmp(last_key_press, "down") == 0)
        {
            if (robot_y < grid_y)
            {
                clear_robot_from_grid_coordinate(robot_x, robot_y, win);
                robot_y++;
                print_robot_from_grid_coordinate(robot_x, robot_y, win);
                if (check_if_got_human(robot_x, robot_y, human_x, human_y, number_of_human, danger_zone_x, danger_zone_y, &danger_zone_number, win) == 1)
                {
                    continue;
                }
                else
                {
                    check_if_danger_zone(robot_x, robot_y, danger_zone_x, danger_zone_y, danger_zone_number, win);
                }
                wrefresh(win);
            }
            else
            {
                print_robot_from_grid_coordinate(robot_x, robot_y, win);
                if (immunity == false)
                {
                    lives--;
                    immunity = true;
                    immunity_start_time = time(NULL);
                }
            }
        }
        // check if the robot is immune
        // if it is immune check if the immunity time is up
        // if it is up set the immunity flag to false
        if (immunity == true)
        {
            if (time(NULL) - immunity_start_time > immunity_time)
            {
                immunity = false;
            }
        }
        // check if the game over flag is true
        // if it is true show the game over screen
        // the reaseon I dont put this below is to make the robot dead charcdter show up
        if (game_over_flag == true)
        {
            sleep(5);
            clear();
            endwin();
            game_over();
            return 0;
        }
        // check if the lives is 0
        // if it is 0 set the game over flag to true and show the game over screen on the next loop
        if (lives == 0)
        {
            game_over_flag = true;
            move(1 + (robot_y - 1) * 2, 1 + (robot_x - 1) * (strlen(robot) + 1));
            addstr("       ");
            move(1 + (robot_y - 1) * 2, 1 + (robot_x - 1) * (strlen(robot) + 1));
            addstr(".[x x].");
            show_detail();
        }
        // calculate the speed
        speed = 500 - (level * 50);
        // check if the speed is less than the max speed
        // if it is less than the max speed sleep the program for the speed
        if (speed > max_speed)
        {
            usleep(speed * 1000);
        }
        else
        {
            usleep(max_speed * 1000);
        }
    }

    endwin();
    return 0;
}
