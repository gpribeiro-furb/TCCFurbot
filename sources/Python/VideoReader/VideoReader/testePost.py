import requests

url = 'http://192.168.1.7/post-data'
data = {'data': 'Hello ESP32'}

response = requests.post(url, data=data)
print(response.text)


url = 'http://192.168.1.7/get'
response = requests.get(url, data=data)
print(response.text)
