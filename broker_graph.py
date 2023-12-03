import paho.mqtt.client as paho
import matplotlib.pyplot as plt 
import time as t

eixo_t = [] #faz uma lista para o tempo
eixo_temp = [] #faz uma lista para a temperatura
eixo_umid = [] #lista para a umidade
eixos_max = 10 #tamanho do máximo de valores plotáveis
eixos_tmn_atual = 0

def mod_globais(nv_t: str, nv_temp: str, nv_umid: str):
    global eixo_t 
    global eixo_temp
    global eixo_umid
    global eixos_max
    global eixos_tmn_atual

    print(eixo_t)
    print(" ")
    print(eixo_temp)
    print(" ")
    print(eixo_umid)
    print('\n')

    nv_temp = list(nv_temp)
    nv_temp.pop()
    nv_temp.pop()
    

    nv_umid = list(nv_umid)
    nv_umid.pop()


    if eixos_tmn_atual < eixos_max:
        eixo_t.append(nv_t)
        eixo_temp.append(float("".join(map(str, nv_temp))))
        eixo_umid.append(float("".join(map(str, nv_umid))))
        eixos_tmn_atual += 1
    else:
        eixo_t.append(nv_t)
        eixo_temp.append(float("".join(map(str, nv_temp))))
        eixo_umid.append(float("".join(map(str, nv_umid))))
        eixo_t.pop(0)
        eixo_temp.pop(0)
        eixo_umid.pop(0)


def plota_graf_temp_umid(titulo_temp: str, titulo_umid: str):

    global eixo_t 
    global eixo_temp
    global eixo_umid
    global eixos_max
    global eixos_max
    global eixos_tmn_atual

    if (eixos_max == eixos_tmn_atual):

        figure, TempUmid = plt.subplots(2)
        
        TempUmid[0].plot(eixo_t, eixo_temp) #plota temperatura
        TempUmid[0].set_title(titulo_temp)
        #TempUmid[0].xlabel("Horário da medição") #plota temperatura
        #TempUmid[0].ylable("Temperatura medida")
        
        TempUmid[1].plot(eixo_t, eixo_umid) #plota temperatura
        TempUmid[1].set_title(titulo_umid)
        #TempUmid[1].xlabel("Horário da medição") #plota temperatura
        #TempUmid[1].ylable("Umidade medida")

        plt.show()


def plota_msg(msg: str): #FORMATO = b'{<data>-<hora>} {<temp>°C} {<umid>%}' | Onde temp tem 4 dígitos, e umid tbm, data é separada por '/' e tem 2 dig cada, e a hora tem 2 dig e é separada por ':'
    
    seletor = 0 # (0: procura data) (1: procura hora) (2: procura temperatura) (3: procura umidade)
    edit = False #levanta ao início da informação
    data = ''
    horas = ''
    temp = ''
    umid = ''

    print(msg + "\n")

    for i in range(len(msg)):

        ###data-----------------------------------------------
        if ((msg[i] != '-') and (seletor == 0) and edit): #transfere a string para data enquanto procura o final da informação
            data += msg[i]

        elif ((msg[i] == '-') and (seletor == 0) and edit):
            seletor = 1 #vai para a prx etapa
            edit = False

        ###horas----------------------------------------------
        if ((msg[i] != '}') and (seletor == 1) and edit):
            horas += msg[i]

        elif ((msg[i] == '}') and (seletor == 1) and edit):
            seletor = 2
            edit = False

        ###temperatura----------------------------------------
        if ((msg[i] != '}') and (seletor == 2) and edit):
            temp += msg[i]

        elif ((msg[i] == '}') and (seletor == 2) and edit):
            seletor = 3
            edit = False

        ###umidade--------------------------------------------
        if ((msg[i] != '}') and (seletor == 3) and edit):
            umid += msg[i]

        elif ((msg[i] == '}') and (seletor == 3) and edit):
            seletor = -1 #nada acontece

        #procura o início da informação  
        if (((msg[i] == '{') and (seletor != -1)) or ((msg[i] == '-') and (seletor == 1))):
            edit = True

    
    graf_temp_tit = 'Gráfico de temperatura - ' + data
    graf_umid_tit = 'Gráfico da umidade - '+ data

    mod_globais(horas, temp, umid)

    plota_graf_temp_umid(graf_temp_tit, graf_umid_tit)
    



def on_message_plot(client, userdata, msg):
    print("preparando para plotar: ") 
    plota_msg(str(msg.payload.decode('utf-8')))


def on_subscribe(client, userdata, mid, granted_qos):
    print("Subscribed: "+str(mid)+" "+str(granted_qos))

def on_connect(client, userdata, flags, rc):
    print('CONNACK received with code %d.' % (rc))

#parâmentros gerais

descanso = 30 #tempo para cada leitura

id_do_cliente = 'PcNic'
topico = 'ardNic/DHT11/leituras'
hive_server = 'broker.hivemq.com'
porta = 1883
com_time = 180 #tempo de conexão máximo com broker


#conexão com o servidor já loopa naturalmente
cliente = paho.Client(client_id= id_do_cliente)


cliente.on_connect = on_connect

cliente.connect(hive_server, porta, keepalive= com_time)

cliente.on_subscribe = on_subscribe
cliente.on_connect = on_connect
cliente.on_message = on_message_plot

cliente.subscribe(topico)

while True:
    
    cliente.loop()
    t.sleep(30)
