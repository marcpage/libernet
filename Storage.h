#ifndef __Storage_h__
#define __Storage_h__

#include "os/Path.h"
#include "os/Sqlite3Plus.h"
#include "os/File.h"
#include "os/DateTime.h"
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
			io::Path _location(const String &name);
	};

	inline Container::Container(const io::Path &path):_top(path.mkdirs()), _dbPath(path+"meta.sqlite3"), _parts(path+"parts"), _db(_dbPath) {
		_db.exec("CREATE TABLE IF NOT EXISTS data ("
					"name VARCHAR(256), "
					"size INT, "
					"first_time REAL, "
					"start_time REAL, "
					"get_time REAL, "
					"get_count INT, "
					"del_time REAL, "
					"del_count INT, "
					"put_time REAL, "
					"put_count INT);"
		);
	}
	inline void Container::put(const String &name, const String &data) {
		io::Path				location= _location(name);
		const bool				fileExisted= location.isFile();
		String					now= std::to_string(dt::DateTime().seconds());
		Sqlite3::DB::Results	results;

		if (!fileExisted) {
			io::File	file(location, io::File::Binary, io::File::ReadWrite);

			file.write(data);
		}
		// TODO: when logging added log when size is not the same as data size
		_db.exec("SELECT put_count FROM data WHERE name LIKE '"+name+"';", &results);
		if (results.size() == 0) {
			Sqlite3::DB::Row row;

			row["size"]= data.length();
			row["name"]= name;
			row["first_time"]= now;
			row["start_time"]= now;
			row["put_time"]= now;
			row["put_count"]= "1";
			_db.addRow("data", row);
		} else {
			String command= "UPDATE data SET ";
			command+= "put_time = '" + now + "', ";
			if (!fileExisted) {
				command+= "start_time = '" + now + "', ";
			}
			command+= "put_count = '" + std::to_string(std::stoi(results[0]["put_count"])) + "' ";
			command+= "WHERE name LIKE '"+name+"';";
			_db.exec(command);
		}
	}
	inline Container::String &Container::get(const String &name, String &buffer) {
		io::Path				location= _location(name);
		const bool				fileExisted= location.isFile();
		String					now= std::to_string(dt::DateTime().seconds());
		Sqlite3::DB::Results	results;

		if (fileExisted) {
			io::File	file(location, io::File::Binary, io::File::ReadOnly);

			file.read(buffer);
		} else {
			buffer.clear();
		}
		_db.exec("SELECT get_count FROM data WHERE name LIKE '"+name+"';", &results);
		if (results.size() > 0) {
			String	command= "UPDATE data SET ";
			String	getCount;
			try {
				getCount= std::to_string(std::stoi(results[0]["get_count"]));
			} catch(const std::exception&) {
				getCount= "1";
			}
			command+= "get_time = '" + now + "', ";
			command+= "get_count = '" + getCount + "' ";
			command+= "WHERE name LIKE '"+name+"';";
			_db.exec(command);
		}
		return buffer;
	}
	inline Container::StringList &Container::find(const String &like, int count, StringList &list) {
		// TODO implement
		return list;
	}
	inline io::Path Container::_location(const String &name) {
		io::Path path= _parts + name.substr(0,2) + name.substr(2,4) + name.substr(4,6);

		path.mkdirs();
		return path + name;
	}

}

#endif // __Storage_h__
