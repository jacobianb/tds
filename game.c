#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
 
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
 
#define BUF_LEN 1024
#define ACTIVE_TEXTS_LEN 10

struct text_t {
        char *text;
        SDL_Surface *text_surface;
        SDL_Texture *texture;
        SDL_Rect dstrect; /* destination rectangle */
};

struct cell_t {
        int x;
        int y;
        char *hover_text;
};

struct data_t {
        int w;
        int h;
        int cell_size;
        int fullscreen;

        char *name;
        char *server_ip;
        int server_port;

        /* these variables are set by the server. */
        int map_w;
        int map_h;
        int max_update_rate;
};

void parse_cfg_file(FILE *cfg_file, struct data_t *cfg);
int set_buffer(char *buf, char *str);

/* SDL stuff */
int init(struct data_t *cfg);
void quit();
struct text_t init_text(int x, int y, int r, int g, int b, int a, char *text);
void render_map(struct cell_t **map, struct data_t *cfg);

//The window we'll be rendering to
SDL_Window *window = NULL;

//The window renderer
SDL_Renderer *renderer = NULL;

//Globally used font
TTF_Font *gFont = NULL;

int main(int argc, char **argv) {
        if (argc < 2) {
                fprintf(stderr, "usage: 2drust <config file>\n");
                return 1;
        }

        struct data_t cfg;
        cfg.w = 0;
        cfg.h = 0;
        cfg.cell_size = 0;
        cfg.fullscreen = 0;
        cfg.name = NULL;
        cfg.server_ip = NULL;

        FILE *cfg_file = NULL;
        cfg_file = fopen(argv[1], "r");
        if (cfg_file == NULL) {
                fprintf(stderr, "ERR: file read error on config file\n");
                return 1;
        }
        parse_cfg_file(cfg_file, & cfg);
        fclose(cfg_file);

        puts("CLIENT VARIABLES:");
        printf("width = %d\n", cfg.w);
        printf("height = %d\n", cfg.h);
        printf("fullscreen = %d\n", cfg.fullscreen);
        printf("name = %s\n", cfg.name);
        printf("server_ip = %s\n", cfg.server_ip);
        printf("server_port = %d\n", cfg.server_port);
        puts("");

        /* server connection stuff */
        char buf[BUF_LEN];
        int sock = socket(PF_INET, SOCK_DGRAM, 0);
        struct sockaddr_in server;
        server.sin_family = AF_INET;
        server.sin_port = htons(cfg.server_port); /* htons converts between little and big endian */
        server.sin_addr.s_addr = inet_addr(cfg.server_ip); /* retarded format. it's their spec, not mine. */
        memset(server.sin_zero, '\0', sizeof server.sin_zero); /* this is also retarded */
        int server_size = sizeof(server);
        int len = 0;

        len = set_buffer(buf, "a"); /* "a" is for initial request, server returns its variables */

        /* for some reason they insist that the struct be const. this means we have to do
        the autistic casting you see below if you want it to compile. */
        sendto(sock, buf, len, 0, (struct sockaddr*) & server, server_size); /* retarded. */
        memset(buf, '\0', BUF_LEN); /* clear the buffer */
        len = recvfrom(sock, buf, BUF_LEN, 0, NULL, NULL);
        //printf("SERVER SAYS: %s\n", buf);
        puts("SERVER VARIABLES:");
        sscanf(buf, "%dx%d:%d", &cfg.map_w, &cfg.map_h, &cfg.max_update_rate);
        printf("map_w = %d\nmap_h = %d\nmax_update_rate = %d\n", cfg.map_w, cfg.map_h, cfg.max_update_rate);

        /* now take the data we got from the server and
        allocate a 2d array from it */
        struct cell_t **map = NULL;
        map = malloc(cfg.map_w * sizeof(struct cell_t * ));
        if (map == NULL) {
                fprintf(stderr, "ERR: malloc failed");
                return 1;
        }
        int i;
        int j;
        for (i = 0; i < cfg.map_w; i++) {
                map[i] = malloc(cfg.map_h * sizeof(struct cell_t));
                if (map[i] == NULL) {
                        fprintf(stderr, "ERR: malloc failed");
                        return 1;
                }
                for (j = 0; j < cfg.map_h; j++) {
                        map[i][j].x = cfg.cell_size * i;
                        map[i][j].y = cfg.cell_size * j;
                        map[i][j].hover_text = NULL;
                }
        }

        /*
        for (i = 0; i < cfg.map_w; i++) {
            for (j = 0; j < cfg.map_h; j++) {
                printf("%d,%d\n", map[i][j].x, map[i][j].y);
        }
    } */

        /* SDL Stuff */
        /*************/
        init(&cfg); /* this inits all the sdl crap */

        struct text_t *active_text[ACTIVE_TEXTS_LEN] = {
                0
        };

        struct text_t t1 = init_text(50, 50, 255, 0, 255, 255, "Kill yourself.");
        active_text[0] = &t1;

        struct text_t t2 = init_text(150, 150, 255, 0, 0, 255, "F for fullscreen toggle\n Y for chat.");
        active_text[1] = &t2;

        int should_quit = 0;
        //int picker = 0;
        SDL_Event event;

        int fullscreen = 0;
        int txt_input_enabled = 0;
        char text_buf[256];

        while (!should_quit) {
                /*SDL_GetMouseState(&x, &y);
                x /= 25;
                y /= 25;
                printf("hovering on box %d,%d\n", x, y); */
                while (SDL_PollEvent(&event) != 0) {
                        /* User requests quit */
                        switch (event.type) {
                        case SDL_QUIT:
                                should_quit = 1;
                                break;
                        case SDL_KEYDOWN:
                                if (txt_input_enabled) {
                                        const char *key = SDL_GetKeyName(event.key.keysym.sym);
                                        if (event.key.keysym.sym == SDLK_SPACE) {
                                                key = " ";
                                        }
                                        if (strlen(key) == 1) {
                                                printf("key: %s\n", key);
                                                strncat(text_buf, key, 1);
                                        }
                                }
                                switch (event.key.keysym.sym) {
                                case SDLK_UP:
                                        active_text[0]->dstrect.y--;
                                        break;
                                case SDLK_DOWN:
                                        active_text[0]->dstrect.y++;
                                        break;
                                case SDLK_LEFT:
                                        active_text[0]->dstrect.x--;
                                        break;
                                case SDLK_RIGHT:
                                        active_text[0]->dstrect.x++;
                                        break;
                                default:
                                        /* any other key */
                                        break;
                                } /* terminate switch */
                                break;
                        case SDL_KEYUP:
                                switch (event.key.keysym.sym) {
                                case SDLK_f:
                                        if (fullscreen) {
                                                SDL_SetWindowFullscreen(window, 0);
                                                fullscreen = 0;
                                        } else {
                                                SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
                                                fullscreen = 1;
                                        }
                                        break;
                                case SDLK_y:
                                        if (!txt_input_enabled) {
                                                txt_input_enabled = 1;
                                        }
                                        break;
                                case SDLK_RETURN:
                                        if (txt_input_enabled) {
                                                txt_input_enabled = 0;
                                                printf("%s\n", text_buf);
                                        }
                                }
                                break;
                                /* case SDL_TEXTINPUT:
                                      Add new text onto the end of our text *
                                     printf("KEY:%s\n",event.text.text);
                                     strcat(text, event.text.text);
                                     break; */
                        } /* terminate event switch */
                }
                /* Clear screen */
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_RenderClear(renderer);
                /*
                       if ( (SDL_GetTicks() % 50) == 0) {
                           int x = active_text[0]->dstrect.x, y = active_text[0]->dstrect.y;
                           SDL_DestroyTexture(active_text[0]->texture);
                           SDL_FreeSurface(active_text[0]->text_surface);
                           if (picker == 0) {
                              
                               t1 = init_text(x, y, 100, 0, 255, 255, "None of that halo");
                               picker = 1;
                           } else {
                               t1 = init_text(x, y, 255, 0, 255, 255, "No games.");
                               picker = 0;
                           }
                           active_text[0] = &t1;
                       }
                      
 
                       for (i = 0; i < ACTIVE_TEXTS_LEN; i++) {
                           if (active_text[i] != 0) {
                               SDL_RenderCopy(renderer, active_text[i]->texture, NULL, &active_text[i]->dstrect);
                           }
                       }*/
                render_map(map, &cfg);
                //Update screen
                SDL_RenderPresent(renderer);
                // should_quit = 1;
        } /* terminate game loop */

        quit(); /* quits all the SDL stuff */
        //int j;
        for (i = 0; i < cfg.map_w; i++) {
                free(map[i]);
        }
        free(map);
        free(cfg.name);
        free(cfg.server_ip);
        return 0;
}

/* returns len of info in buffer */
int set_buffer(char *buf, char *str) {
        int i;
        for (i = 0; str[i]; i++) {
                buf[i] = str[i];
        }
        return i;
}

void parse_cfg_file(FILE *cfg_file, struct data_t *cfg) {
        char buf[BUF_LEN];
        char key[BUF_LEN];
        char *tok = NULL;
        int iskey = 1;

        while (fgets(buf, BUF_LEN, cfg_file)) {
                if (buf[0] == '#') {
                        continue;
                } else if ((tok = strstr(buf, "#"))) {
                        tok[0] = '\0';
                }
                tok = strtok(buf, ":\n ");
                iskey = 1;
                do {
                        if (iskey) {
                                strncpy(key, tok, strlen(tok) + 1); /* +1 for that pesky NULL */
                                iskey = 0;
                        } else {
                                switch (key[0]) {
                                case 'n':
                                        cfg->name = malloc(strlen(tok) + 1); /* +1 for that pesky NULL */
                                        if (cfg->name == NULL) {
                                                fprintf(stderr, "ERR: malloc failed on name\n");
                                                exit(EXIT_FAILURE);
                                        }
                                        strncpy(cfg->name, tok, strlen(tok) + 1); /* +1 for that pesky NULL */
                                        break;
                                case 's':
                                        cfg->server_ip = malloc(strlen(tok) + 1); /* +1 for that pesky NULL */
                                        if (cfg->server_ip == NULL) {
                                                fprintf(stderr, "ERR: malloc failed on server ip\n");
                                                exit(EXIT_FAILURE);
                                        }
                                        strncpy(cfg->server_ip, tok, strlen(tok) + 1); /* +1 for that pesky NULL */
                                        break;
                                case 'p':
                                        sscanf(tok, "%d", &cfg->server_port);
                                        break;
                                case 'w':
                                        sscanf(tok, "%d", &cfg->w);
                                        break;
                                case 'h':
                                        sscanf(tok, "%d", &cfg->h);
                                        break;
                                case 'c':
                                        sscanf(tok, "%d", &cfg->cell_size);
                                        break;
                                case 'f':
                                        sscanf(tok, "%d", &cfg->fullscreen);
                                        if (cfg->fullscreen > 0) {
                                                cfg->fullscreen = 1;
                                        } else {
                                                cfg->fullscreen = 0;
                                        }
                                        break;
                                }
                        }
                } while ((tok = strtok(NULL, ":\n ")));
        }
}

struct text_t init_text(int x, int y, int r, int g, int b, int a, char *text) {
        struct text_t thetext;
        SDL_Color color = {
                r,
                g,
                b,
                a
        };
        thetext.text_surface = TTF_RenderText_Solid(gFont, text, color);
        if (thetext.text_surface == NULL) {
                printf("Unable to create text surface! SDL_ttf Error: %s\n", TTF_GetError());
        }
        thetext.texture = SDL_CreateTextureFromSurface(renderer, thetext.text_surface);

        int w, h;
        SDL_QueryTexture(thetext.texture, NULL, NULL, &w, &h);
        SDL_Rect dstrect = {
                x,
                y,
                w,
                h
        };
        thetext.dstrect = dstrect;

        return thetext;
}

void render_map(struct cell_t **map, struct data_t *cfg) {
        SDL_Rect rect;
        int i;
        int j;
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        for (i = 0; i < cfg->map_w; i++) {
                for (j = 0; j < cfg->map_h; j++) {
                        rect.x = map[i][j].x;
                        rect.y = map[i][j].y;
                        rect.w = cfg->cell_size;
                        rect.h = cfg->cell_size;
                        SDL_RenderDrawRect(renderer, &rect);
                }
        }
}

int init(struct data_t *cfg) {
        /* Initialize SDL */
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
                printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
                return 1;
        }

        /* Create window */
        window = SDL_CreateWindow("IQ Simulator 2k17", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, cfg->w, cfg->h, SDL_WINDOW_SHOWN);
        if (window == NULL) {
                printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
                return 1;
        }

        /* Create renderer for window */
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (renderer == NULL) {
                printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
                return 1;
        }
        if (TTF_Init() == -1) {
                printf("SDL_ttf could not initialized! SDL_ttf Error: %s\n", TTF_GetError());
                return 1;
        }
        gFont = TTF_OpenFont("DejaVuSans.ttf", 18);
        if (gFont == NULL) {
                printf("Failed to load font! SDL_ttf Error: %s\n", TTF_GetError());
                return 1;
        }

        return 0;
}

void quit() {
        /* Destroy renderer and window */
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        window = NULL;
        renderer = NULL;

        /* Quit SDL subsystems */
        TTF_Quit();
        SDL_Quit();
}