#ifndef EXTERN_H
#define EXTERN_H

extern time_t encendido;
extern unsigned int num_id;

extern Config *config;

extern std::list <User*> users;
extern std::list <Servidor*> servidores;
extern std::list <Chan*> canales;
extern std::list <Memo*> memos;

void mayuscula (std::string &str);
bool checknick (const std::string nick);
bool checkchan (const std::string chan);

extern int shouldNotExit;

#define strlcpy(x, y, z) strncpy(x, y, z)

#endif
