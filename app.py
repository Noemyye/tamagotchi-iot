from flask import Flask, render_template, request, jsonify
from flask_socketio import SocketIO, emit
import sqlite3
import json
import os

app = Flask(__name__)
app.config['SECRET_KEY'] = 'secret!'
socketio = SocketIO(app, cors_allowed_origins="*")

DB_NAME = "tamagotchi.db"

# Initialize database
def init_db():
    conn = sqlite3.connect(DB_NAME)
    c = conn.cursor()
    c.execute('''
        CREATE TABLE IF NOT EXISTS tamagotchis (
            id TEXT PRIMARY KEY,
            name TEXT,
            character TEXT,
            hunger INTEGER DEFAULT 0,
            happiness INTEGER DEFAULT 100,
            cleanliness INTEGER DEFAULT 100,
            alive INTEGER DEFAULT 1 CHECK (alive IN (0,1))
        )
    ''')
    conn.commit()
    conn.close()

@app.route('/')
def index():
    return render_template("index.html")

@app.route('/api/tamagotchi/update', methods=['POST'])
def update_tamagotchi():
    data = request.json
    print(f"Received data: {data}")
    conn = sqlite3.connect(DB_NAME)
    c = conn.cursor()
    
    # Check if tamagotchi exists
    c.execute("SELECT id FROM tamagotchis WHERE id = ?", (data['id'],))
    exists = c.fetchone()
    
    if exists:
        c.execute("""
            UPDATE tamagotchis 
            SET hunger = ?, happiness = ?, cleanliness = ?
            WHERE id = ?
        """, (data['hunger'], data['fun'], data['cleanliness'], data['id']))
    else:
        c.execute("""
            INSERT INTO tamagotchis (id, name, character, hunger, happiness, cleanliness, alive)
            VALUES (?, ?, ?, ?, ?, ?, ?)
        """, (
            data['id'], data['name'], data['character'], 
            data['hunger'], data['fun'], data['cleanliness'],
            1 if data.get('alive', True) else 0
        ))
    
    conn.commit()
    conn.close()
    
    # Emit update to all connected clients
    socketio.emit('tamagotchi_update', data)
    return jsonify({"status": "success"})

@app.route('/api/tamagotchi/<tama_id>')
def get_tamagotchi(tama_id):
    conn = sqlite3.connect(DB_NAME)
    c = conn.cursor()
    c.execute("SELECT * FROM tamagotchis WHERE id = ?", (tama_id,))
    tama = c.fetchone()
    conn.close()
    
    if tama:
        return jsonify({
            "id": tama[0],
            "name": tama[1],
            "character": tama[2],
            "hunger": tama[3],
            "happiness": tama[4],
            "cleanliness": tama[5],
            "alive": tama[6]
        })
    return jsonify({"error": "Tamagotchi not found"}), 404

@socketio.on('connect')
def handle_connect():
    print('Client connected')

@socketio.on('disconnect')
def handle_disconnect():
    print('Client disconnected')

if __name__ == '__main__':
    init_db()
    socketio.run(app, host='0.0.0.0', debug=True)
