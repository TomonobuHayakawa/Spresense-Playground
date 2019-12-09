#ifndef CODE_TABLE_H_
#define CODE_TABLE_H_

class CodeTypeClass
{

public:
  CodeTypeClass(int key, char* name, int* ommit, int num):
    m_key(key),
    m_scale_name(name),
    m_ommit_notes(ommit),
    m_ommit_num(num) {};

  int   get_key()  {return m_key;}
  char* get_name() {return m_scale_name;}
  int*  get_notes(){return m_ommit_notes;}
  int   get_size() {return m_ommit_num;}

private:
  const int   m_key;
  const char* m_scale_name;
  const int*  m_ommit_notes;
  const int   m_ommit_num;
};


extern CodeTypeClass E_Major;
extern CodeTypeClass C_Okinawa;

#endif
