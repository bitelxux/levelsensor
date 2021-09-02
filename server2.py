#!/usr/bin/python3

from flask import Flask, Markup, render_template

app = Flask(__name__)

@app.route('/')
def main():
     return "I'm alive!"

@app.route('/add/<int:value>')
def add(value):
    return "The value %d has been gladly added" % value

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=8888, threaded=True)
