from flaskDashboard import app
import platform

PORT = 8050
if __name__ == "__main__":
    print(f"Connect with the browser to http://{platform.node()}:{PORT}")
    app.run(debug=True, host="0.0.0.0", port=PORT, use_reloader=False)
