from flaskDashboard import app
import platform

PORT = 8050
if __name__ == "__main__":
    print("Connect to the browser to")
    print(f"http://{platform.node()}:{PORT}")
    app.run(debug=True, host="0.0.0.0", port=PORT)
