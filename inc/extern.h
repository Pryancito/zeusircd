#ifndef EXTERN_H
#define EXTERN_H

extern time_t encendido;

extern Config *config;

extern List<Socket*> sock;
extern List<User*> users;
extern List<Servidor*> servidores;
extern List<Chan*> canales;
extern List<UserChan*> usuarios;
extern List<BanChan*> bans;
extern List<Memo*> memos;

void mayuscula (std::string &str);

extern std::locale loc;
extern std::mutex uchan_mtx, user_mtx, sock_mtx, ban_mtx, serv_mtx;

#define strlcpy(x, y, z) strncpy(x, y, z)

#endif
