# GameCapstoneDesign
2022-2 GameCapstoneDesign 

## Runner & Action Game with Hybrid Character Model
### 작품 배경 및 개요
* 게임에 존재하는 다양한 주변 환경과의 상호작용에 대응되는 많은 애니메이션이 필요하지만, 모든 애니메이션을 미리 준비하기 어려움
* 상호작용에 있어서 물체가 가지는 특성에 따라 kinematics와 physics를 이용하여 동적으로 반응하는 애니메이션 생성 방법 제시

### 개발 방법
* "Generating Upper-Body Motion for Real-Time Characters Making their Way through Dynamic Environments" 논문 참고하여 Unreal engine에 Upper body motion 구현 (https://edualvarado.com/generating-upper-body-motion/)
* Unreal engine physics 이용

### 작품 내용
* Hybrid Character Model
  * 외부 물리적 현상을 인식하기 위한 Visibility Model 구현, IK를 적용한 환경 데이터와 목표 지점에 따른 Kinematic Skeleton 생성
  * Stiffness 값 조절을 통한 Physics 기반 충돌 구현, Unreal Physics Simulation 을 통해 동적으로 반응하는 애니메이션 적용
* Runner Game
  * 장애물이 존재하는 맵에서 플레이어는 장애물을 피하거나 Safety Region을 이용하여 장애물에 닿지 않고 제한 시간 내에 도착지점까지 도달하는 것이 목표
  * 장애물이 물리적 현상(e.g. push, swing)에 따른 신체 부위 별 Physics 기반 충돌 처리로 게임 내 지연 시간 발생
  * 제한 시간을 초과하면 전투장에서 패널티 적용
* Action Game
  * 전투 시 적(AI)과 공격, 피격에 따른 상호작용이 발생하였을 때 Physics 기반 전투 시스템 구현
  * 신체 부위 별 Physics Hit, Simulation을 이용해 상대방을 전투장 밖으로 벗어나도록 하는 것이 목표

### 스크린샷
* Kinematic Animation VS Physics Animation

![fig2-1](https://user-images.githubusercontent.com/86725870/210034740-34cd663b-07c9-46fa-b700-d30e9e00a8ec.png)
![fig2-2](https://user-images.githubusercontent.com/86725870/210034741-c9a2eaaa-c74c-464d-a1dc-2c9c9c22bd2a.png)
* Upper body Simulation

![상호작용4](https://user-images.githubusercontent.com/86725870/210034813-6fe3e957-6703-4d51-9fcd-b5c8e783752a.PNG)
![상호작용1](https://user-images.githubusercontent.com/86725870/210034820-a97503ab-855a-4320-97c1-3a0b35339cb3.PNG)
<img src="https://user-images.githubusercontent.com/86725870/210034803-6c8ee7a1-e5aa-4f81-b03e-5febd4eaed36.png"  width="400" height="400"/>
<img src="https://user-images.githubusercontent.com/86725870/210035068-21cc17c7-014a-4fdf-83de-d0da338a6d16.PNG"  width="400" height="400"/>

### 영상 링크
https://www.youtube.com/watch?v=fBdjfd2bkEw
### 팀원
* 김민정 https://github.com/mmindoong
* 임재원 https://github.com/adfa0117
* 임형준(팀장) https://github.com/GbLeem
