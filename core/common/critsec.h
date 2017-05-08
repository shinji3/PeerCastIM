#ifndef _CRITSEC_H
#define _CRITSEC_H

#include "sys.h"

// �N���e�B�J���Z�N�V�������}�[�N����B�R���X�g���N�^�[�Ń��b�N���擾
// ���A�f�X�g���N�^�ŊJ������B
class CriticalSection
{
public:
    CriticalSection(WLock& lock)
        : m_lock(lock)
    {
        m_lock.on();
    }

    ~CriticalSection()
    {
        m_lock.off();
    }

    WLock& m_lock;
};

#endif
