#ifndef __Storage_h__
#define __Storage_h__

#include "os/Path.h"
#include "os/Sqlite3Plus.h"
#include "os/File.h"
#include "os/DateTime.h"
#include <string>
#include <map>
#include <algorithm>

namespace store {

	class Container {
		public:
			typedef std::string String;
			struct Metadata {
				Metadata():size(0),have(false),score(0.0) {}
				Metadata(long size, bool have):size(size),have(have),score(0.0) {}
				Metadata(long size, bool have, double score):size(size),have(have),score(score) {}
				long size;
				bool have;
				double score;
			};
			typedef std::map<String,Metadata> NameList;
			Container(const io::Path &path);
			~Container() {}
			void put(const String &name, const String &data);
			String &get(const String &name, String &buffer);
			String get(const String &name) {String buffer; return get(name, buffer);}
			bool del(const String &name);
			bool has(const String &name);
			NameList &find(const String &like, int count, NameList &list);
			NameList find(const String &like, int count) {NameList list; return find(like, count, list);}
			NameList &removable(const String &like, int count, NameList &list);
			NameList removable(const String &like, int count) {NameList list; return removable(like, count, list);}
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
		_db.exec("SELECT put_count,size FROM data WHERE name LIKE '"+name+"';", &results);
		if (results.size() == 0) {
			Sqlite3::DB::Row row;

			row["size"]= std::to_string(data.length());
			row["name"]= name;
			row["first_time"]= now;
			row["start_time"]= now;
			row["put_time"]= now;
			row["put_count"]= "1";
			row["get_count"]= "0";
			row["del_count"]= "0";
			_db.addRow("data", row);
		} else {
			String command= "UPDATE data SET ";

			command+= "put_time = '" + now + "', ";
			if (!fileExisted) {
				command+= "start_time = '" + now + "', ";
				for (Sqlite3::DB::Results::iterator i= results.begin(); i != results.end(); ++i) {
					for (Sqlite3::DB::Row::iterator r= i->begin(); r != i->end(); ++r) {
						printf("'%s' = '%s'\t", r->first.c_str(), r->second.c_str());
					}
					printf("\n");
				}
				if (static_cast<unsigned long>(std::stol(results[0]["size"])) != data.length()) {
					command+= "size = '" + std::to_string(data.length()) + "', ";
				}
			}
			command+= "put_count = '" + std::to_string(std::stoi(results[0]["put_count"]) + 1) + "' ";
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
				getCount= std::to_string(std::stoi(results[0]["get_count"]) + 1);
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
	inline bool Container::del(const String &name) {
		io::Path location= 		_location(name);
		String					now= std::to_string(dt::DateTime().seconds());
		Sqlite3::DB::Results	results;

		try {
			_location(name).unlink();
		} catch(const std::exception&) {
			return false;
		}
		_db.exec("SELECT del_count,size FROM data WHERE name LIKE '"+name+"';", &results);
		if (results.size() > 0) {
			String command= "UPDATE data SET ";
			String	delCount;

			try {
				delCount= std::to_string(std::stoi(results[0]["del_count"]) + 1);
			} catch(const std::exception&) {
				delCount= "1";
			}
			command+= "del_time = '" + now + "', ";
			command+= "del_count = '" + delCount + "' ";
			command+= "WHERE name LIKE '"+name+"';";
			_db.exec(command);
		}
		return true;
	}
	inline bool Container::has(const String &name) {
		io::Path	location= _location(name);

		return location.isFile();
	}
	inline Container::NameList &Container::find(const String &like, int count, NameList &list) {
		Sqlite3::DB::Results	results;
		String::size_type		numberOfCharacters= like.length();
		NameList::size_type		lastCount= 0;

		list.clear();
		do	{
			_db.exec("SELECT name,size FROM data WHERE name LIKE '"+like.substr(0, numberOfCharacters)+"%' LIMIT "+std::to_string(count)+";", &results);
			if (results.size() > lastCount) {
				lastCount= results.size();
				for (Sqlite3::DB::Results::iterator row= results.begin(); (list.size() < static_cast<unsigned int>(count)) && (row != results.end()); ++row) {
					NameList::iterator found= list.find((*row)["name"]);

					if (found == list.end()) {
						list[(*row)["name"]]= Metadata(std::stoi((*row)["size"]), _location((*row)["name"]).isFile());
					}
				}
			}
			numberOfCharacters-= 1;
		} while ( (numberOfCharacters > 0) && (list.size() < static_cast<unsigned int>(count)) );
		if (numberOfCharacters > 0) {
			numberOfCharacters-= 1;
		}
		_db.exec("SELECT name,size FROM data WHERE name LIKE '%"+like.substr(0, numberOfCharacters)+"';", &results);
		return list;
	}
	inline Container::NameList &Container::removable(const String &like, int count, NameList &list) {
		printf("removable('%s', %d)\n", like.c_str(), count);
		Sqlite3::DB::Results	results;
		String					now= std::to_string(dt::DateTime().seconds());
		String::size_type		numberOfCharacters= like.length();
		const unsigned int		expectedCount= count;

		list.clear();
		//(put_count + get_count - del_count) * (now - start_time + now - first_time + now - put_time + now - get_time - now + del_time) * size
		//(put_count + get_count - del_count) * (3.0 * now - start_time - first_time - put_time - get_time + del_time) * size
		// may be NULL: get_count, del_count, get_time, del_time
		printf("%s\n", ("SELECT name, size,"
					"(put_count + get_count - del_count)"
					"* (3.0 * " + now + " - start_time - first_time - put_time"
						"+ CASE WHEN get_time IS NULL THEN " + now + " ELSE get_time END + CASE WHEN del_time IS NULL THEN " + now + " ELSE del_time END)"
					" * size AS score "
					"FROM data ORDER BY score DESC LIMIT " + std::to_string(count * 2) + ";").c_str());
		_db.exec("SELECT name, size,"
					"(put_count + get_count - del_count)"
					"* (3.0 * " + now + " - start_time - first_time - put_time"
						"+ CASE WHEN get_time IS NULL THEN " + now + " ELSE get_time END"
						"+ CASE WHEN del_time IS NULL THEN " + now + " ELSE del_time END) / 31536000.0"
					" * size / 1048576.0 AS score "
					"FROM data ORDER BY score ASC LIMIT " + std::to_string(count * 2) + ";", &results);
		for (Sqlite3::DB::Results::iterator row= results.begin(); row != results.end(); ++row) {
			list[(*row)["name"]]= Metadata(std::stoi((*row)["size"]), _location((*row)["name"]).isFile(), std::stod((*row)["score"]));
			printf("%s size=%s score=%s\n", (*row)["name"].c_str(), (*row)["size"].c_str(), (*row)["score"].c_str());
		}
		while ( (numberOfCharacters > 0) && (list.size() > expectedCount) ) {
			for (NameList::iterator name= list.begin(); (list.size() > expectedCount) && (name != list.end());) {
				const String::size_type most= std::min(name->first.length(), numberOfCharacters);

				if (name->first.substr(0, most) == like.substr(0, most)) {
					name= list.erase(name);
				} else {
					++name;
				}
			}
			numberOfCharacters-= 1;
		}
		return list;
	}
	inline io::Path Container::_location(const String &name) {
		const String		parts= name + (name.length() < 6 ? String(6 - name.length(), '_') : String());
		const io::Path		path= _parts + parts.substr(0,2) + parts.substr(2,2) + parts.substr(4,2);

		path.mkdirs();
		return path + name;
	}

}

#endif // __Storage_h__
