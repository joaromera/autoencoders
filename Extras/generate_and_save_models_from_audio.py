# -*- coding: utf-8 -*-

import argparse


def make_autoencoder(filename, epochs=1):
    # Utils
    from glob import glob
    import tqdm
    from pathlib import Path

    import datetime

    # Numeros
    import numpy as np
    from scipy.interpolate import interp1d

    # Redes neuronales
    from keras.utils.vis_utils import model_to_dot, plot_model
    from keras.models import Sequential, Model
    from keras.layers import Input, Dense, Conv2D, MaxPooling2D, UpSampling2D, Flatten, Reshape, BatchNormalization
    from keras.initializers import RandomNormal
    from keras.optimizers import RMSprop, Adam
    from keras import backend as K
    from keras import regularizers
    from keras.datasets import mnist
    import keras as keras
    import tensorflow as tf
    import tensorflowjs as tfjs

    # Machine learning
    from sklearn.manifold import TSNE
    from sklearn.model_selection import train_test_split
    from sklearn.decomposition import PCA

    # Audio
    import librosa
    import librosa.display

    Sclip = -60
    hop_length_ms = 10
    duration = 60*2

    sr = 22050
    path = filename

    hop_length = int(hop_length_ms/1000*sr)
    win_length = hop_length*4

    X = []
    y = []
    phases = []

    for i,filename in enumerate(glob(path)):
        x,sr = librosa.load(filename, sr=sr, mono=True,duration=duration)
        x = np.trim_zeros(x)
        F = librosa.stft(x,n_fft=win_length, hop_length=hop_length).T
        phases.append(np.angle(F))
        S = 10*np.log10(np.abs(F)**2)
        S = S.clip(Sclip, None)-Sclip
        y.append(np.ones(S.shape[0])*i)
        X.append(S)

    phases = np.vstack(phases)
    X = np.vstack(X)
    Xmax = X.max()
    X = X/Xmax
    y = np.hstack(y)

    n_features = X.shape[1]

    # Separamos en train y val

    X_train, X_val, y_train, y_val = train_test_split(X, y,  test_size=0.05, shuffle=True)

    # Armamos un autoencoder denso de 8 capas

    K.clear_session()

    input_mag = Input(shape=(n_features,))

    kr_init = keras.initializers.glorot_normal()

    layers = [512,256,128,64,32,16,8]
    latent_dim = layers[-1]

    x = input_mag
    for l in layers:
        x = BatchNormalization()(x)
        x = Dense(l, activation='relu', kernel_initializer=kr_init)(x)
    encoded = x

    encoder = Model(input_mag, encoded)
    latent_inputs = Input(shape=(latent_dim,), name='z')
    x = latent_inputs
    for l in layers[::-1][1:]:
        x = BatchNormalization()(x)
        x = Dense(l, activation='relu', kernel_initializer=kr_init)(x)
    decoded = Dense(n_features, activation='relu', kernel_initializer=kr_init)(x)

    decoder = Model(latent_inputs, decoded, name='decoder')

    outputs = decoder(encoder(input_mag))
    autoencoder = Model(input_mag, outputs, name='autoencoder')

    opt = RMSprop(0.005)
    opt = Adam(0.002, clipnorm=0.5)

    autoencoder.compile(optimizer=opt, loss='mse', metrics=['mse'])

    logdir = Path("logs", datetime.datetime.now().strftime("%Y%m%d-%H%M%S"))
    tensorboard_callback = keras.callbacks.TensorBoard(logdir, histogram_freq=1)

    silence = np.zeros([int(X_train.shape[0]*0.1),X_train.shape[1]])

    autoencoder.fit(np.vstack([silence, X_train]), np.vstack([silence, X_train]),
                    epochs=epochs,
                    batch_size=128,
                    shuffle=True, 
                    validation_data= (X_val,X_val),
                    callbacks=[tensorboard_callback])
    
    # save models
    encoder.save('encoder')
    decoder.save('decoder')
    autoencoder.save('autoencoder')

def main():
    """Do stuff"""

    parser = argparse.ArgumentParser(description='Create a simple autoencoder, train it with some input wave file and output the saved models')
    parser.add_argument('-a', '--audio', help="WAVE file to build upon the autoencoder", required=True)
    parser.add_argument('-e', '--epochs', help="Number of epochs for fitting", required=False)
    args = parser.parse_args()
    make_autoencoder(args.audio, int(args.epochs))


if __name__ == "__main__":
    main()

