using namespace std;

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include "imdb.h"

const char *const imdb::kActorFileName = "actordata";
const char *const imdb::kMovieFileName = "moviedata";

struct keyAndFile {
	const void* key;
	const void* file;
};

int actorCmp(const void *elem1, const void *elem2) {
	keyAndFile *strct = (keyAndFile*)elem1;
	const void* file = strct -> file;
	const string actor1 = *(string*)(strct -> key);
	string actor2 = "";
	for(char* ch = (char*)file + *(int*)elem2; *ch != '\0'; ch++) {
		actor2 = actor2 + *ch;
	}
	if(actor1 == actor2) return 0;
	if(actor1 < actor2) return -1;
	else return 1;
}

int movieCmp(const void *elem1, const void *elem2) {
	keyAndFile *strct = (keyAndFile*)elem1;
	const void* file = strct -> file;
	const film movie1 = *(film*)(strct -> key);
	film movie2;
	movie2.title = "";
	char* ch = (char*)file + *(int*)elem2;
	for(; *ch != '\0'; ch++) {
		movie2.title = movie2.title + *ch;
	}
	movie2.year = *(char*)(ch + 1) + 1900;
	if(movie1 == movie2) return 0;
	if(movie1 < movie2) return -1;
	else return 1;
}

imdb::imdb(const string& directory)
{
	const string actorFileName = directory + "/" + kActorFileName;
	const string movieFileName = directory + "/" + kMovieFileName;
	
	actorFile = acquireFileMap(actorFileName, actorInfo);
	movieFile = acquireFileMap(movieFileName, movieInfo);
}

bool imdb::good() const
{
	return !( (actorInfo.fd == -1) || 
			(movieInfo.fd == -1) ); 
}

bool imdb::getCredits(const string& player, vector<film>& films) const { 
	keyAndFile actor;
	actor.file = actorFile;
	actor.key = &player;
	void* actorPtr = bsearch(&actor, (int*)actorFile + 1, *(int*)actorFile, sizeof(int), actorCmp);
	if(actorPtr == NULL) return false;
	char* ptr = (char*)actorFile + *(int*)actorPtr;
	ptr = (char*)ptr + player.length() + 1 + (1 - (player.length() % 2));
	short numOfMovies = *(short*)ptr;
	ptr = ptr + 2;
	if((int)(ptr - (char*)actorPtr) % 4 != 0) ptr = (char*)ptr + 2;
	
	for(int i = 0; i < numOfMovies; i++) {
		int curMovieOffset = *((int*)ptr + i);
		film curFilm;
		curFilm.title = "";
		char* temp = (char*)movieFile + curMovieOffset;
		for(; *temp != '\0'; temp++) {
		curFilm.title = curFilm.title + *temp;
		}
		curFilm.year = *(char*)(temp + 1) + 1900;
		films.push_back(curFilm);
	}
	return true;
}

bool imdb::getCast(const film& movie, vector<string>& players) const {
	keyAndFile mov;
	mov.file = movieFile;
	mov.key = &movie;
	void* filmPtr = bsearch(&mov, (int*)movieFile + 1, *(int*)movieFile, sizeof(int), movieCmp);
	if(filmPtr == NULL) return false;
	char* ptr = (char*)movieFile + *(int*)filmPtr;
	ptr = ptr + movie.title.length() + 1 + 1 + (movie.title.length() % 2);
	short numOfActors = *(short*)ptr;
	ptr = ptr + 2;
	if((ptr - (char*)filmPtr) % 4 != 0) ptr = ptr + 2;
	
	for(int i = 0; i < numOfActors; i++) {
		int curActorOffset = *((int*)ptr + i);
		string curActorName = "";
		for(char* temp = (char*)actorFile + curActorOffset; *temp != '\0'; temp++) {
		curActorName = curActorName + *temp;
		}
		players.push_back(curActorName);
	}
	return true;
}

imdb::~imdb()
{
	releaseFileMap(actorInfo);
	releaseFileMap(movieInfo);
}

// ignore everything below... it's all UNIXy stuff in place to make a file look like
// an array of bytes in RAM.. 
const void *imdb::acquireFileMap(const string& fileName, struct fileInfo& info)
{
	struct stat stats;
	stat(fileName.c_str(), &stats);
	info.fileSize = stats.st_size;
	info.fd = open(fileName.c_str(), O_RDONLY);
	return info.fileMap = mmap(0, info.fileSize, PROT_READ, MAP_SHARED, info.fd, 0);
}

void imdb::releaseFileMap(struct fileInfo& info)
{
	if (info.fileMap != NULL) munmap((char *) info.fileMap, info.fileSize);
	if (info.fd != -1) close(info.fd);
}