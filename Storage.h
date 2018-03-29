#ifndef __Storage_h__
#define __Storage_h__

#include "os/Path.h"
#include "os/Sqlite3Plus.h"
#include <string>
#include <vector>

namespace store {

	class Container {
		public:
			typedef std::string String;
			typedef std::vector<String> StringList;
			Container(const io::Path &path);
			~Container() {}
			void put(const String &name, const String &data);
			String &get(const String &name, String &buffer);
			String get(const String &name) {String buffer; return get(name, buffer);}
			StringList &find(const String &like, int count, StringList &list);
			StringList find(const String &like, int count) {StringList list; return find(like, count, list);}
		private:
			io::Path 	_top;
			io::Path 	_dbPath;
			io::Path 	_parts;
			Sqlite3::DB	_db;
			Container(const Container &other);
			Container &operator=(const Container &other);
	};

	inline Container::Container(const io::Path &path):_top(path), _dbPath(path+"meta.sqlite3"), _parts(path+"parts"), _db(_dbPath) {
		_db.exec("CREATE TABLE IF NOT EXISTS `data` ("
					"`name` VARCHAR(256), "
					"`size` INT, "
					"`first_time` INT, "
					"`start_time` INT, "
					"`get_time` INT, "
					"`get_count` INT, "
					"`del_time` INT, "
					"`del_count` INT, "
					"`put_time` INT, "
					"`put_count` INT);"
		);
	}
	inline void Container::put(const String &name, const String &data) {
		
	}
	inline String &Container::get(const String &name, String &buffer) {
	}
	inline StringList &Container::find(const String &like, int count, StringList &list) {
	}

}

#endif // __Storage_h__
