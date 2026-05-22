import { createContext } from 'react';
import { LocalizationProvider } from './LocalizationProvider';

export const LocalizationContext = createContext(new LocalizationProvider(""));
