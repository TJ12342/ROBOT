from flask import Blueprint, jsonify, request
import httpx

robot_bp = Blueprint('robot', __name__)

url = "http://192.168.98.1"

@robot_bp.route('/command/<int:code>', methods=['GET'])
def command(code):
  resp = httpx.get(f"{url}/{code}/on")

  if resp.is_success is False:
    return jsonify({"message": "发送出现错误"}), 400
  
  return jsonify({"message": "发送成功"}), 200