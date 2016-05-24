#include "netplay-text.h"

TextCommand::TextCommand(magic_t m) :magic{m}
{
  TextCommand::Registrar.Register(this);
  *this << Registration::SuperMagic << magic;
}

TextCommand::magic_t TextCommand::Magic() { return magic; }

void TextCommand::Enable(bool e)
{
  enabled = e;
}

void TextCommand::Disable()
{
  enabled = false;
}

void TextCommand::Toggle()
{
  Enable(!enabled);
}

bool TextCommand::Enabled() { return enabled; }

void TextCommand::Send(const std::string& message)
{
  omsg += message;
  MDFNI_NetplayRaw(omsg.c_str(), omsg.size());
  omsg.clear();
  *this << Registration::SuperMagic << magic;
}

static inline bool magic_valid(const char *msg, TextCommand::magic_t magic)
{
  printf ("magic == %04x\n", magic);
  return *reinterpret_cast<const TextCommand::magic_t *>(msg) == magic;
}

bool TextCommand::Registration::SuperMagicValid(const char *msg)
{
  return magic_valid(msg, SuperMagic);
}

bool TextCommand::MagicValid(const char* msg)
{
  return magic_valid(msg, magic);
}

void TextCommand::LoadPacket(const char* str, uint32 len)
{
  imsg = std::string(str, len);
}

std::string TextCommand::magic2str(magic_t magic)
{
  std::string s;
  const char *m = reinterpret_cast<const char *>(&magic);
  s += *(m++);
  s += *m;
  return s;
}

int TextCommand::Registration::Register(TextCommand* tc)
{
  commands.push_back(tc);
  // assert all magic are unique
  return 0;
}

void TextCommand::Registration::Process(
  const char *nick, const char *msg, uint32 len, bool &display)
{
  // for (int i=0; i < len; i++)
  //   printf("0x%02x ", static_cast<unsigned char>(msg[i]));

  if (!SuperMagicValid(msg))
    return;
  // iterate through plugins until a match has been found (returns true)
  for (auto& cmd : commands)
  {
    if (!cmd->MagicValid(&msg[sizeof(magic_t)]))
      continue;
    if (!cmd->Enabled())
      continue;
    if (cmd->Process(nick, &msg[sizeof(magic_t)*2], len, display))
      return;
  }
}

TextCommand::Registration TextCommand::Registrar {};