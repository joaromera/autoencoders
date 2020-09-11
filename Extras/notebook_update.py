# {'Xmax':Xmax,'Sclip':Sclip, 'hop_length':hop_length, 'Zmax':Zmax, 'Zmin':Zmin, 'window': hanning(win_length) }

Z = [0,0,0,0,0,0,0,0]
win_length = 440*2
rfft_size = (win_length / 2) + 1

N = 1000
samples = (N - 1) * hop_length + win_length

audio = zeros(samples)

for n in range(N):
  Y_predict = decoder.predict(Z)

  # db a amplitud
  for i in range(rfft_size):
    phase = rand()*2*pi
    y_aux = sqrt(10**((Y_predict[i]*Xmax+Sclip)/10))
    Y_real[i] = y_aux*cos(phase)
    Y_imag[i] = y_aux*sin(phase)

  audio_n = irfft(Y_real, Y_imag)

  for i in range(win_length):
    audio_n[i] = audio_n[i]*window[i]

  for i in range(win_length):
    audio[n*hop_length+i] += audio_n[i]
