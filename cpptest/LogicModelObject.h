#ifndef __LOGICMODELOBJECT_H__
#define __LOGICMODELOBJECT_H__

typedef object_id_t long;

class LogicModelObject {

 private:

  object_id_t object_id;

  std::string name;
  std::string description;

 public:

  virtual LogicModelObject() : object_id(0) {};
  virtual LogicModelObject(object_id_t oid) : object_id(oid);

  virtual LogicModelObject(string object_name, 
			   string object_description) :
    object_id(0),
    name(object_name),
    description(object_description);

  virtual LogicModelObject(object_id_t oid, 
			   string object_name, 
			   string object_description) :
    object_id(oid),
    name(object_name),
    description(object_description);

  virtual ~LogicModelObject() {} ;

  virtual void set_name(string const& name) {
    this->name = name;
  }

  virtual void set_description(string const & description) {
    this->description = description;
  }

  virtual string const & get_name() const {
    return name;
  }

  virtual string const & get_description const {
    return description;
  }

  virtual void get_object_id() const {
    return object_id;
  }

};

#endif
