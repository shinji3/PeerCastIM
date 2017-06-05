#ifndef _ENV_H
#define _ENV_H

#include <vector>
#include <string>

// ���ϐ��̃r���_�[�N���X�B
class Environment
{
public:
    Environment();
    Environment(char *src[]);
    ~Environment();

    void set(const std::string&, const std::string&);
    void unset(const std::string& key);
    std::string get(const std::string&) const;
    bool hasKey(const std::string&) const;
    std::vector<std::string> keys() const;
    size_t size() const { return m_vars.size(); }

    std::vector<std::string> m_vars;

    char const ** m_env;
    // ���̓I�u�W�F�N�g�����ɍ쐬����A�|�C���^�[���Ԃ����B�I�u�W�F
    // �N�g�ɕύX����������ƃf�[�^�͖����������B
    char const ** env();
};

#endif
