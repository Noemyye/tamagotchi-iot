from app import app, socketio, init_db

if __name__ == '__main__':
    init_db()
    print("Starting Flask server on 0.0.0.0:8080")
    print("This will make the server accessible from other devices on your network")
    print("Your ESP32 should connect to this server using the IP address of this computer")
    socketio.run(app, host='0.0.0.0', port=8080, debug=True) 