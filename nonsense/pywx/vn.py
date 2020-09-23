from wxpy import *
bot = Bot()
vn = bot.friends().search('铁头娃')[0]

msg = {'你好，我是Martins3的机器人管家，这条消息并没有被我的主人读取，如果你有着急的事情，可以直接打电话13269711736，拜拜了',
        '主人现在不在，你可以访问我主人的主页: https://github.com/Martins3/Whisper'}

time = 0

@bot.register(vn)
def reply_my_friend(msg):
    if time == 0:
        time = time + 1
        return msg[0]
    else:
        return msg[1]
