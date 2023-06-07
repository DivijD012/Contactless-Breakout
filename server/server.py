from flask import Flask, render_template
import requests
serv = Flask(__name__)

@serv.route('/getrank', methods=['GET'])
def getrank():
    uri_ae = 'http://127.0.0.1:8080/~/in-cse/CAE63995516/?rcn=4'
    headers = {
        'X-M2M-Origin': 'admin:admin',
        'Content-type': 'application/json'
    }
    data = requests.get(uri_ae, headers=headers)
    data = data.json()
    return data
    

@serv.route('/getdata/<gameid>', methods=['GET'])
def getdata(gameid):
    uri_ae = f'http://127.0.0.1:8080/~/in-cse/in-name/GameData/{gameid}/?rcn=4'
    headers = {
        'X-M2M-Origin': 'admin:admin',
        'Content-type': 'application/json'
    }
    data = requests.get(uri_ae, headers=headers)
    return data.json()
   
@serv.route('/', methods=['GET'])
def root():
    return render_template('analysis.html')
   
if __name__ == '__main__':
    serv.run()