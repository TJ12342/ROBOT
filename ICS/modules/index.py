from flask import Blueprint, jsonify, request, render_template

index_bp = Blueprint('index', __name__)

@index_bp.route('/', methods=['GET'])
def welcome():
  return render_template('welcome.html')


@index_bp.route('/index', methods=['GET'])
def index():
  return render_template('index.html')


@index_bp.route('/record', methods=['GET'])
def record():
  return render_template('record.html')


@index_bp.route('/home', methods=['GET'])
def home():
  return render_template('home.html')


@index_bp.route('/shop', methods=['GET'])
def shop():
  return render_template('shop.html')


@index_bp.route('/cart', methods=['GET'])
def cart():
  return render_template('cart.html')


@index_bp.route('/order', methods=['GET'])
def order():
  return render_template('order.html')