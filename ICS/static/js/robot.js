let indexLangObj = {
  'zh': {
    HEAD_MOVEMENT: '头部动作',
    HAND_MOVEMENT: '手部动作',
    BODY_MOVEMENT: '身体动作',
    LOOK_UP: '抬头',
    LOOK_DOWN: '低头',
    LOOK_LEFT: '左扭',
    LOOK_RIGHT: '右扭',
    HAND_SHAKE: '握手',
    WAVE: '招手',
    LEFT_TILT: '左倾',
    RIGHT_TILT: '右倾',
    RAISE: '抬升',
    SERVICE: '服务',
    SIT_DOWN: '下蹲',
    SIT: '坐下',
    DANCE: '跳舞',
    RESET: '复位',
    STOP: '停止'
  },
  'en': {
    HEAD_MOVEMENT: 'Head Movement',
    HAND_MOVEMENT: 'Hand Movement',
    BODY_MOVEMENT: 'Body Movement',
    LOOK_UP: 'Look Up',
    LOOK_DOWN: 'Look Down',
    LOOK_LEFT: 'Look Left',
    LOOK_RIGHT: 'Look Right',
    HAND_SHAKE: 'Hand Shake',
    WAVE: 'Wave',
    LEFT_TILT: 'Left Tilt',
    RIGHT_TILT: 'Right Tilt',
    RAISE: 'Raise',
    SERVICE: 'Service',
    SIT_DOWN: 'Sit Down',
    SIT: 'Sit',
    DANCE: 'Dance',
    RESET: 'Reset',
    STOP: 'Stop'
  }
}

async function sendCommand(code) {
  try {
    let res = await fetch(`http://192.168.98.1/${code}/on`, {
      method: 'GET'
    });

    if (res.ok) {
      let data = await res.json();
      console.log(data);
    } else {
      console.log('出现错误');
    }
  } catch (error) {
    console.error('网络请求失败:', error);
  }
}
function initTitle() {
  if (localStorage.getItem('user_data')) {
    let user = JSON.parse(localStorage.getItem('user_data'))
    document.querySelector('.hello-title').textContent = `Hello，${user.username}`
  }
}

initTitle()
