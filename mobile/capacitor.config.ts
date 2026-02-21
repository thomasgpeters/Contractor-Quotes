import type { CapacitorConfig } from '@capacitor/cli';

const config: CapacitorConfig = {
  appId: 'com.contractorquotes.mobile',
  appName: 'Contractor Quotes',
  webDir: 'dist',
  server: {
    // During development, point to Vite dev server
    // url: 'http://192.168.1.x:3000',
    // cleartext: true,
  },
  plugins: {
    Keyboard: {
      resize: 'body',
      resizeOnFullScreen: true,
    },
    StatusBar: {
      style: 'dark',
      backgroundColor: '#1a73e8',
    },
  },
};

export default config;
