#include <iostream>
#include <unistd.h>
#include <string> 
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <queue>

using namespace std;

class Dirls {
	public:
		Dirls(queue<string> pathQueue, bool aflag, bool dflag, bool fflag, bool lflag, bool hflag); 
		void Execute();

	private: 
		bool aflag;
		bool dflag;
		bool fflag;
		bool lflag;
		bool hflag;
		queue<string> pathQueue;

		void PrintUsage();
		void LongDirectoryListing(char* path); 
		void IncludeDotFiles(char* path); 
		void OnlyTheDirectory(char* path); 
		void RecursiveDirectoryListing(char* name); 

		string GetFileLongName(char* path); 
};

Dirls::Dirls(queue<string> pathQueue, bool aflag, bool dflag, bool fflag, bool lflag, bool hflag) {
	this->pathQueue = pathQueue;
	this->aflag = aflag;
	this->dflag = dflag;
	this->fflag = fflag;
	this->lflag = lflag;
	this->hflag = hflag;
}

void Dirls::Execute() {
	bool showPathName = this->pathQueue.size() > 1; 

	while (!this->pathQueue.empty()) {
		char* path = (char*) pathQueue.front().c_str(); 

		if (showPathName) {
			cout << path << ":" << endl; 
		}

		if (this->hflag) {
			this->PrintUsage(); 
			pathQueue.pop();
			continue;
		}

		if (this->lflag) {
			this->LongDirectoryListing(path); 
			pathQueue.pop();
			continue;
		}

		if (this->aflag) {
			this->IncludeDotFiles(path); 
			pathQueue.pop();
			continue;
		}

		if (this->dflag) {
			this->OnlyTheDirectory(path); 
			pathQueue.pop();
			continue;
		}

		//no flags are enabled except maybe fflag
		this->RecursiveDirectoryListing(path);
		pathQueue.pop();
	}
}

void Dirls::PrintUsage() {
	cout << "Usage: dirls [(-[adflh]+) (dir)]*\n\t-a: include dot files\n\t-f: follow symbolic links\n\t-d: only this directory\n\t-l: long form\n\t-h: prints this message" << endl; 
}

string Dirls::GetFileLongName(char* path) {
	struct stat fileStat; 
	lstat(path, &fileStat); 

	uid_t userId = fileStat.st_uid;
	gid_t groupId = fileStat.st_gid;

	struct passwd* user = getpwuid(userId);
	struct group* group = getgrgid(groupId); 

	// generation of string copied from https://stackoverflow.com/questions/10323060/printing-file-permissions-like-ls-l-using-stat2-in-c
	string permissionString = string((S_ISDIR(fileStat.st_mode)) ? "d" : "-")+ ( (fileStat.st_mode & S_IRUSR) ? "r" : "-") + ( (fileStat.st_mode & S_IWUSR) ? "w" : "-") + ( (fileStat.st_mode & S_IXUSR) ? "x" : "-") + ( (fileStat.st_mode & S_IRGRP) ? "r" : "-") + ( (fileStat.st_mode & S_IWGRP) ? "w" : "-") + ( (fileStat.st_mode & S_IXGRP) ? "x" : "-") + ( (fileStat.st_mode & S_IROTH) ? "r" : "-") + ( (fileStat.st_mode & S_IWOTH) ? "w" : "-") + ( (fileStat.st_mode & S_IXOTH) ? "x" : "-");

	return permissionString + " " + to_string(fileStat.st_size) + " " + user->pw_name + " " + group->gr_name + " " + path; 
}

void Dirls::LongDirectoryListing(char* path) {
	DIR* dir; 

	if (path == NULL) {
		dir = opendir("."); 
	} else {
		//if d flag enabled just print info about it
		if (this->dflag) {
			cout << this->GetFileLongName(path) << endl; 
			return; 
		}

		dir = opendir(path); 
	}

	if (dir == NULL) {
		if (!fopen(path, "r")) {
			cout << "dirls: cannot access " << path <<": No such file or directory"<< endl; 
			return; 
		}

		cout << this->GetFileLongName(path) << endl; 
		return; 
	}

	struct dirent *dirent; 

	struct stat fileStat; 
	lstat(path, &fileStat); 
	//avoiding following symlinks unless f flag is enabled
	if (S_ISLNK(fileStat.st_mode) && !this->fflag) {
		cout << this->GetFileLongName(path) << endl << endl; 

		return; 
	}

	while((dirent = readdir(dir)) != NULL) {
		struct stat fileStat; 

		lstat(dirent->d_name, &fileStat);

		uid_t userId = fileStat.st_uid;
		gid_t groupId = fileStat.st_gid;

		struct passwd* user = getpwuid(userId);
		struct group* group = getgrgid(groupId); 

		// generation of string copied from https://stackoverflow.com/questions/10323060/printing-file-permissions-like-ls-l-using-stat2-in-c
		string permissionString = string((S_ISDIR(fileStat.st_mode)) ? "d" : "-")+ ( (fileStat.st_mode & S_IRUSR) ? "r" : "-") + ( (fileStat.st_mode & S_IWUSR) ? "w" : "-") + ( (fileStat.st_mode & S_IXUSR) ? "x" : "-") + ( (fileStat.st_mode & S_IRGRP) ? "r" : "-") + ( (fileStat.st_mode & S_IWGRP) ? "w" : "-") + ( (fileStat.st_mode & S_IXGRP) ? "x" : "-") + ( (fileStat.st_mode & S_IROTH) ? "r" : "-") + ( (fileStat.st_mode & S_IWOTH) ? "w" : "-") + ( (fileStat.st_mode & S_IXOTH) ? "x" : "-");

		//dont include '.' or '..' unless a flag is enabled
		if ((string(dirent->d_name) != "." && string(dirent->d_name) != "..") || this->aflag) {
			cout << permissionString << " " << fileStat.st_size << " " << user->pw_name << " " << group->gr_name << " " << dirent->d_name << endl; 
		}
	}

	cout << endl; 

	closedir(dir); 
}

void Dirls::IncludeDotFiles(char* path) {
	DIR* dir; 

	if (path == NULL) {
		dir = opendir("."); 
	} else {
		//if d flag enabled just print the path 
		if (this->dflag) {
			cout << path << endl; 
			return; 
		}
		
		dir = opendir(path); 
	}

	if (dir == NULL) {
		if (!fopen(path, "r")) {
			cout << "dirls: cannot access " << path <<": No such file or directory"<< endl; 
			return; 
		}

		cout << path << endl;
		return; 
	}

	struct dirent *dirent; 

	struct stat fileStat; 
	lstat(path, &fileStat); 

	//avoiding following symlinks unless fflag is enabled
	if (S_ISLNK(fileStat.st_mode) && !this->fflag) {
		cout << path << endl; 
		return;
	}


	while((dirent = readdir(dir)) != NULL) {
		cout << dirent->d_name << " "; 
	}

	cout << endl << endl; 

	closedir(dir); 
}

// this is an enhanced version of
// https://stackoverflow.com/questions/8436841/how-to-recursively-list-directories-in-c-on-linux
// that version lists directory until it encounters a subdirectory, then lists the subdirectory
// this version lists the contents of the entire directory then moves on to subdirectory - like ls -R
// queue of directories was used to accomplish the task
void Dirls::RecursiveDirectoryListing(char* path) {
	DIR *dir;
	struct dirent *dirent;

	if (path == NULL) {
		path = (char*)".";
		dir = opendir("."); 
	} else {
		dir = opendir(path); 
	}	

	if (dir == NULL) {
		if (!fopen(path, "r")) {
			cout << "dirls: cannot access " << path <<": No such file or directory"<< endl; 
			return; 
		}

		cout << path << endl;
		return;
	}

	cout << string(path) + ":"<< endl; 
	queue<string> directoryList; 
	struct stat fileStat; 
	lstat(path, &fileStat); 

	//must not read symbolic links unless fflag is enabled
	while ((!(S_ISLNK(fileStat.st_mode)) || this->fflag) && (dirent = readdir(dir)) != NULL) {
		if (dirent->d_type == DT_DIR) {
			//prevent infinite loop
			if (string(dirent->d_name) == "." || string(dirent->d_name) == "..") {
				continue;
			}
			string newPath = string(path) + "/" + dirent->d_name;
			cout << dirent->d_name << " "; 
			directoryList.push(newPath); 
		} else {
			cout << dirent->d_name << " "; 
		}
	}

	cout << endl << endl;

	while (!directoryList.empty()) {
		this->RecursiveDirectoryListing((char*)directoryList.front().c_str());

		directoryList.pop();
	}

	closedir(dir);	
}

void Dirls::OnlyTheDirectory(char* path) {
	if (path == NULL) {
		cout << "." << endl;
		return; 
	}

	cout << path << endl;
}


int main(int argc, char *argv[]) {
	queue<string> pathQueue; 
	int c; 
	bool aflag = false, dflag = false, fflag = false, lflag = false, hflag = false;

	while ((c = getopt(argc, argv, "adflh")) != -1) {
		switch (c) {
			case 'a':
				aflag = true;
				break;
			case 'd':
				dflag = true;
				break;
			case 'f':
				fflag = true;
				break;
			case 'l':
				lflag = true;
				break;
			case 'h':
				hflag = true;
				break;
			default:
				hflag = true; 
				break;
		}
	}

	//add rest of the arguments to the pathQueue
	for (int i=optind; i<argc; i++) {
		pathQueue.push(argv[i]);
	}
	
	if (pathQueue.empty()) {
		pathQueue.push(".");
	}

	Dirls dirls = Dirls(pathQueue, aflag, dflag, fflag, lflag, hflag);
	dirls.Execute(); 
	return 0;
}
